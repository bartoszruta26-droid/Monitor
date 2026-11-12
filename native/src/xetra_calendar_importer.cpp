#include "monitor/market_data_calendar.hpp"
#include "monitor/xetra_calendar_importer.hpp"

#include <libxml/HTMLparser.h>
#include <libxml/tree.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <map>
#include <limits>
#include <optional>
#include <memory>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace monitor::market {
namespace {

std::string node_text(xmlNode* node) {
    xmlChar* raw = xmlNodeGetContent(node);
    if (raw == nullptr) return {};
    std::string value(reinterpret_cast<const char*>(raw));
    xmlFree(raw);
    return value;
}

std::string normalized(std::string value) {
    std::string out;
    bool pending_space = false;
    for (unsigned char ch : value) {
        if (std::isspace(ch) != 0) {
            pending_space = !out.empty();
        } else {
            if (pending_space) out.push_back(' ');
            pending_space = false;
            out.push_back(static_cast<char>(std::tolower(ch)));
        }
    }
    return out;
}

void collect_elements(xmlNode* node, const char* name, std::vector<xmlNode*>& out) {
    for (xmlNode* current = node; current != nullptr; current = current->next) {
        if (current->type == XML_ELEMENT_NODE &&
            xmlStrcasecmp(current->name, BAD_CAST name) == 0) out.push_back(current);
        collect_elements(current->children, name, out);
    }
}

std::vector<xmlNode*> direct_cells(xmlNode* row) {
    std::vector<xmlNode*> cells;
    for (xmlNode* child = row->children; child != nullptr; child = child->next) {
        if (child->type == XML_ELEMENT_NODE &&
            (xmlStrcasecmp(child->name, BAD_CAST "th") == 0 ||
             xmlStrcasecmp(child->name, BAD_CAST "td") == 0)) cells.push_back(child);
    }
    return cells;
}

unsigned month_number(const std::string& name) {
    static const std::map<std::string, unsigned> months{
        {"jan",1},{"january",1},{"feb",2},{"february",2},{"mar",3},{"march",3},
        {"apr",4},{"april",4},{"may",5},{"jun",6},{"june",6},{"jul",7},{"july",7},
        {"aug",8},{"august",8},{"sep",9},{"september",9},{"oct",10},{"october",10},
        {"nov",11},{"november",11},{"dec",12},{"december",12}
    };
    const auto it = months.find(normalized(name));
    if (it == months.end()) throw std::invalid_argument("Nieznany miesiąc w kalendarzu Xetra HTML.");
    return it->second;
}

CivilDate parse_year_date(const std::smatch& match, int expected_year) {
    const bool month_first=match[2].matched;
    const auto month=month_number(month_first ? match[2].str() : match[6].str());
    const auto day=static_cast<unsigned>(std::stoul(month_first ? match[3].str() : match[5].str()));
    const auto year=static_cast<int>(std::stoi(month_first ? match[4].str() : match[7].str()));
    if(expected_year!=0 && year!=expected_year)
        throw std::invalid_argument("Rok daty w źródle Xetra nie odpowiada zakresowi dokumentu.");
    const CivilDate date{year,month,day};
    if(!is_valid_date(date)) throw std::invalid_argument("Niepoprawna data w źródle Xetra.");
    return date;
}

const std::regex& year_date_pattern() {
    static const std::regex pattern(
        R"(((January|February|March|April|May|June|July|August|September|October|November|December|Jan|Feb|Mar|Apr|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+([0-9]{1,2})[,]?\s+([0-9]{4}))|([0-9]{1,2})\s+(January|February|March|April|May|June|July|August|September|October|November|December|Jan|Feb|Mar|Apr|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+([0-9]{4}))",
        std::regex::icase);
    return pattern;
}

CivilDate date_from_cell(const std::string& text,int expected_year) {
    std::sregex_iterator it(text.begin(),text.end(),year_date_pattern()),end;
    if(it==end) throw std::invalid_argument("Brak jednoznacznej daty z rokiem w komórce tabeli Xetra.");
    const auto match=*it;
    ++it;
    if(it!=end) throw std::invalid_argument("Komórka tabeli Xetra zawiera więcej niż jedną datę.");
    return parse_year_date(match,expected_year);
}

std::vector<CivilDate> dates_in_text(const std::string& text,int expected_year=0) {
    std::vector<CivilDate> result;
    for(std::sregex_iterator it(text.begin(),text.end(),year_date_pattern()),end;it!=end;++it)
        result.push_back(parse_year_date(*it,expected_year));
    return result;
}

LocalTimeOfDay parse_clock_time(const std::smatch& match,std::size_t hour_index,std::size_t minute_index) {
    const auto hour=static_cast<unsigned>(std::stoul(match[hour_index].str()));
    const auto minute=static_cast<unsigned>(std::stoul(match[minute_index].str()));
    if(hour>23U || minute>59U)
        throw std::invalid_argument("Godzina sesji Xetra wykracza poza zakres doby.");
    return {hour,minute,0U};
}

void validate_notice_phase_events(const std::vector<VenueSessionPhaseEvent>& events) {
    if(events.empty()) throw std::invalid_argument("Ogłoszenie PDF nie zawiera zdarzeń faz sesji.");
    std::set<std::string> keys;
    std::optional<std::tuple<unsigned,unsigned,unsigned>> previous;
    for(const auto& event:events) {
        const auto key=std::to_string(static_cast<unsigned>(event.kind))+":"+event.scope;
        if(!keys.insert(key).second || event.at.hour>23U || event.at.minute>59U ||
           event.at.second>59U ||
           ((event.kind==VenueSessionPhaseEventKind::TradeAtCloseEnd ||
             event.kind==VenueSessionPhaseEventKind::OffBookServicesEnd) && event.scope.empty()))
            throw std::invalid_argument("Ogłoszenie PDF zawiera powtórzone lub nieprawidłowe zdarzenie fazy.");
        const auto current=std::tuple{event.at.hour,event.at.minute,event.at.second};
        if(previous && current<*previous)
            throw std::invalid_argument("Zdarzenia fazy w ogłoszeniu PDF nie są uporządkowane.");
        previous=current;
    }
}

} // namespace

VenueSessionCalendar parse_xetra_html_calendar(
    std::string_view html, const SourceCalendarDocument& document) {
    validate_source_calendar_document(document);
    if (html.empty() || html.size() > 16U * 1024U * 1024U)
        throw std::invalid_argument("Dokument HTML Xetra jest pusty lub przekracza limit 16 MiB.");
    const int calendar_year=document.valid_from.year;
    if(document.venue!="XETRA" || document.timezone!="Europe/Berlin" ||
       document.valid_from.month!=1U || document.valid_from.day!=1U ||
       document.valid_to_exclusive.year!=calendar_year+1 ||
       document.valid_to_exclusive.month!=1U || document.valid_to_exclusive.day!=1U)
        throw std::invalid_argument("Importer Xetra wymaga pełnego zakresu jednego roku kalendarzowego.");

    using Doc = std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)>;
    Doc parsed(htmlReadMemory(html.data(), static_cast<int>(html.size()),
        document.source_uri.c_str(), "UTF-8",
        HTML_PARSE_NONET | HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING), &xmlFreeDoc);
    if (!parsed) throw std::invalid_argument("Nie można sparsować dokumentu HTML Xetra.");

    std::vector<xmlNode*> tables;
    collect_elements(xmlDocGetRootElement(parsed.get()), "table", tables);
    std::map<std::string, CivilDate> holidays;
    bool found_year_table=false;
    const std::set<std::string> required{
        "new year's day", "good friday", "easter monday", "labor day",
        "christmas eve", "christmas day", "boxing day", "new year's eve"
    };
    for (xmlNode* table : tables) {
        std::vector<xmlNode*> rows;
        collect_elements(table->children, "tr", rows);
        std::size_t year_column = static_cast<std::size_t>(-1);
        for (xmlNode* row : rows) {
            const auto cells = direct_cells(row);
            for(std::size_t i=0;i<cells.size();++i) {
                if(normalized(node_text(cells[i]))==std::to_string(calendar_year)) {
                    if(year_column!=static_cast<std::size_t>(-1) && year_column!=i)
                        throw std::invalid_argument("Tabela Xetra zawiera niejednoznaczną kolumnę roku.");
                    year_column=i;
                }
            }
            if (year_column == static_cast<std::size_t>(-1)) continue;
            found_year_table=true;
            if (cells.size() <= year_column || cells.size() < 2U) continue;
            const std::string label = normalized(node_text(cells.front()));
            if (!required.contains(label)) continue;
            const auto date=date_from_cell(node_text(cells[year_column]),calendar_year);
            if (!holidays.emplace(label, date).second)
                throw std::invalid_argument("Powtórzony wiersz święta w tabeli Xetra HTML.");
        }
        if(found_year_table && holidays.size()==required.size()) break;
    }
    if(!found_year_table || holidays.size()!=required.size())
        throw std::invalid_argument("Tabela Xetra HTML nie zawiera kompletnego zestawu ośmiu wierszy dla wybranego roku.");

    std::set<std::string> holiday_dates;
    for(const auto& [label,date]:holidays) {
        (void)label;
        if(!holiday_dates.insert(format_date(date)).second)
            throw std::invalid_argument("Dwa święta Xetra wskazują tę samą datę.");
    }

    std::vector<xmlNode*> paragraphs;
    collect_elements(xmlDocGetRootElement(parsed.get()), "p", paragraphs);
    std::string paragraph_text;
    for(xmlNode* paragraph:paragraphs) {
        if(!paragraph_text.empty()) paragraph_text.push_back(' ');
        paragraph_text+=normalized(node_text(paragraph));
    }
    std::smatch regular_hours_match;
    const std::regex regular_hours_pattern(
        R"(regular trading hours are from ([0-9]{1,2}):([0-9]{2}) to ([0-9]{1,2}):([0-9]{2}))",
        std::regex::icase);
    if(!std::regex_search(paragraph_text,regular_hours_match,regular_hours_pattern))
        throw std::invalid_argument("Brak jednoznacznych regularnych godzin sesji w dokumencie Xetra.");
    const auto regular_open=parse_clock_time(regular_hours_match,1U,2U);
    const auto regular_close=parse_clock_time(regular_hours_match,3U,4U);
    if(std::tie(regular_open.hour,regular_open.minute)>=std::tie(regular_close.hour,regular_close.minute))
        throw std::invalid_argument("Regularne godziny sesji Xetra są odwrócone lub puste.");

    std::smatch shares_hours_match,structured_hours_match;
    const std::regex shares_hours_pattern(
        R"(shares and exchange traded products ends at ([0-9]{1,2}):([0-9]{2}))",std::regex::icase);
    const std::regex structured_hours_pattern(
        R"(structured products are traded until ([0-9]{1,2}):([0-9]{2}))",std::regex::icase);
    if(!std::regex_search(paragraph_text,shares_hours_match,shares_hours_pattern) ||
       !std::regex_search(paragraph_text,structured_hours_match,structured_hours_pattern))
        throw std::invalid_argument("Brak jednoznacznych godzin sesji świątecznej Xetra.");
    const auto shares_close=parse_clock_time(shares_hours_match,1U,2U);
    const auto structured_close=parse_clock_time(structured_hours_match,1U,2U);
    if(std::tie(shares_close.hour,shares_close.minute)<=std::tie(regular_open.hour,regular_open.minute) ||
       std::tie(structured_close.hour,structured_close.minute)<=std::tie(regular_open.hour,regular_open.minute))
        throw std::invalid_argument("Godziny zakończenia sesji świątecznej Xetra są nieprawidłowe.");

    const std::string notice_marker="in the year "+std::to_string(calendar_year)+" regular stock exchange trading takes place";
    const bool has_notice = std::any_of(paragraphs.begin(), paragraphs.end(), [&](xmlNode* node) {
        return normalized(node_text(node)).find(notice_marker) != std::string::npos;
    });
    if (!has_notice)
        throw std::invalid_argument("Brak semantycznej informacji o handlu w święta wybranego roku.");
    std::vector<xmlNode*> list_items;
    collect_elements(xmlDocGetRootElement(parsed.get()), "li", list_items);
    const std::set<std::string> named_trading{
        "ascension day","whit monday","corpus christi"
    };
    std::map<std::string,CivilDate> actual_trading;
    std::set<std::string> trading_dates;
    for(xmlNode* item:list_items) {
        const auto label=normalized(node_text(item));
        for(const auto& name:named_trading) {
            if(label.find(name)==std::string::npos) continue;
            const auto parsed_dates=dates_in_text(node_text(item));
            if(parsed_dates.size()!=1U)
                throw std::invalid_argument("Data święta z handlem jest niejednoznaczna.");
            if(parsed_dates.front().year!=calendar_year) continue;
            if(!actual_trading.emplace(name,parsed_dates.front()).second ||
               !trading_dates.insert(format_date(parsed_dates.front())).second)
                throw std::invalid_argument("Data święta z handlem jest powtórzona.");
        }
    }
    if(actual_trading.size()!=named_trading.size())
        throw std::invalid_argument("Lista świąt z handlem Xetra jest niekompletna lub nieznana.");

    SourceCalendarDocument source = document;
    std::set<std::string> extraordinary_notice_dates;
    for(xmlNode* paragraph:paragraphs) {
        const auto label=normalized(node_text(paragraph));
        if(label.find("deviating trading hours may apply")==std::string::npos) continue;
        const auto dates=dates_in_text(node_text(paragraph));
        if(dates.size()!=1U)
            throw std::invalid_argument("Komunikat o nadzwyczajnych godzinach Xetra ma niejednoznaczną datę.");
        if(dates.front().year!=calendar_year) continue;
        if(!extraordinary_notice_dates.insert(format_date(dates.front())).second)
            throw std::invalid_argument("Komunikat o nadzwyczajnych godzinach Xetra powtarza datę.");
    }

    VenueSessionCalendar result;
    result.calendar_id=document.document_id;
    result.revision=document.version;
    result.parser_version="xetra-html-"+std::to_string(calendar_year)+"-v1";
    result.source=std::move(source);
    using namespace std::chrono;
    const sys_days begin{year{calendar_year}/January/1};
    const sys_days end{year{calendar_year+1}/January/1};
    for (auto date = begin; date < end; date += days{1}) {
        const year_month_day ymd{date};
        VenueCalendarDay day;
        day.date = CivilDate{static_cast<int>(ymd.year()),
            static_cast<unsigned>(ymd.month()), static_cast<unsigned>(ymd.day())};
        const auto key = format_date(day.date);
        const bool settlement=key==format_date(holidays.at("christmas eve")) ||
                              key==format_date(holidays.at("new year's eve"));
        const bool closed=std::any_of(holidays.begin(),holidays.end(),[&](const auto& entry) {
            return format_date(entry.second)==key && !settlement;
        });
        const unsigned weekday_number = weekday{date}.iso_encoding();
        if (weekday_number >= 6U || closed) {
            day.status = VenueDayStatus::Closed;
        } else if (settlement) {
            day.status = VenueDayStatus::SettlementOnly;
        } else {
            day.status = VenueDayStatus::Trading;
            if(trading_dates.contains(key)) {
                day.product_sessions.push_back({"shares_and_etps",regular_open,shares_close});
                day.product_sessions.push_back({"structured_products",regular_open,structured_close});
            } else {
                day.product_sessions.push_back({"xetra_regular",regular_open,regular_close});
            }
            day.requires_official_notice=extraordinary_notice_dates.contains(key);
        }
        result.days.push_back(std::move(day));
    }
    return result;
}

VenueSessionNotice parse_xetra_session_notice_text(
    std::string_view extracted_text,VenueSessionNotice notice) {
    if(extracted_text.empty() || extracted_text.size()>2U*1024U*1024U)
        throw std::invalid_argument("Tekst komunikatu Xetra jest pusty lub przekracza limit 2 MiB.");
    static const std::regex id_pattern(R"(^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$)");
    static const std::regex source_id_pattern(R"(^sha256:[0-9a-f]{64}$)");
    if(!std::regex_match(notice.notice_id,id_pattern) || notice.revision==0U ||
       !std::regex_match(notice.calendar_id,id_pattern) || normalized(notice.venue)!="xetra" ||
       !std::regex_match(notice.source_object_id,source_id_pattern) ||
       notice.source_uri.rfind("https://",0)!=0 || notice.attribution.empty() ||
       !is_valid_date(notice.effective_date))
        throw std::invalid_argument("Metadane komunikatu PDF Xetra są niekompletne lub nieprawidłowe.");

    const auto text=normalized(std::string(extracted_text));
    auto market_start=text.find("market deutsche börse xetra");
    if(market_start==std::string::npos) market_start=text.find("market deutsche boerse xetra");
    if(market_start==std::string::npos)
        throw std::invalid_argument("Ogłoszenie PDF nie zawiera sekcji rynku Deutsche Börse Xetra.");
    auto market_end=text.find("availability",market_start);
    if(market_end==std::string::npos) market_end=text.size();
    const auto market_section=text.substr(market_start,market_end-market_start);
    if(market_section.find("mic: xetr")==std::string::npos &&
       market_section.find("mic xetr")==std::string::npos)
        throw std::invalid_argument("Sekcja ogłoszenia nie potwierdza MIC XETR.");

    static const char* months[]={"","january","february","march","april","may","june",
        "july","august","september","october","november","december"};
    const auto textual_date=std::to_string(notice.effective_date.day)+" "+
        months[notice.effective_date.month]+" "+std::to_string(notice.effective_date.year);
    const auto iso_date=format_date(notice.effective_date);
    if(text.find(textual_date)==std::string::npos && text.find(iso_date)==std::string::npos)
        throw std::invalid_argument("Ogłoszenie PDF nie potwierdza wskazanej daty obowiązywania.");

    const auto extract_time=[&](const std::string& pattern_text,const char* event_name) {
        const std::regex pattern(pattern_text,std::regex::icase);
        std::smatch match;
        if(!std::regex_search(market_section,match,pattern))
            throw std::invalid_argument(std::string("Ogłoszenie PDF nie zawiera godziny zdarzenia: ")+event_name);
        const unsigned hour12=static_cast<unsigned>(std::stoul(match[1].str()));
        const unsigned minute=static_cast<unsigned>(std::stoul(match[2].str()));
        const auto meridiem=normalized(match[3].str());
        if(hour12<1U || hour12>12U || minute>59U)
            throw std::invalid_argument("Godzina zdarzenia w ogłoszeniu PDF jest nieprawidłowa.");
        unsigned hour=hour12%12U;
        if(meridiem=="p.m.") hour+=12U;
        return LocalTimeOfDay{hour,minute,0U};
    };
    std::vector<VenueSessionPhaseEvent> events;
    events.push_back({VenueSessionPhaseEventKind::PreTradingStart,
        extract_time(R"(from\s+([0-9]{1,2})\.([0-9]{2})\s*(a\.m\.|p\.m\.).{0,100}pre-trading)","pre-trading"),{}});
    events.push_back({VenueSessionPhaseEventKind::RetailEarlyStart,
        extract_time(R"(from\s+([0-9]{1,2})\.([0-9]{2})\s*(a\.m\.|p\.m\.).{0,100}retail early)","retail early"),{}});
    events.push_back({VenueSessionPhaseEventKind::OpeningAuctionCall,
        extract_time(R"(from\s+([0-9]{1,2})\.([0-9]{2})\s*(a\.m\.|p\.m\.).{0,100}opening auction call)","opening auction"),{}});
    events.push_back({VenueSessionPhaseEventKind::IntradayAuctionCall,
        extract_time(R"(from\s+([0-9]{1,2})\.([0-9]{2})\s*(a\.m\.|p\.m\.).{0,100}intraday auction call)","intraday auction"),{}});
    events.push_back({VenueSessionPhaseEventKind::ClosingAuctionCall,
        extract_time(R"(from\s+([0-9]{1,2})\.([0-9]{2})\s*(a\.m\.|p\.m\.).{0,100}closing auction call)","closing auction"),{}});
    events.push_back({VenueSessionPhaseEventKind::ClosingAuctionPriceDeterminationNotBefore,
        extract_time(R"(no earlier than\s+([0-9]{1,2})\.([0-9]{2})\s*(a\.m\.|p\.m\.))","closing auction price"),{}});
    events.push_back({VenueSessionPhaseEventKind::TradeAtCloseEnd,
        extract_time(R"(trade-at-close.{0,180}?(?:end|until)[^0-9]{0,40}([0-9]{1,2})\.([0-9]{2})\s*(a\.m\.|p\.m\.))","Trade-at-Close"),"DAX, MDAX, SDAX"});
    events.push_back({VenueSessionPhaseEventKind::OffBookServicesEnd,
        extract_time(R"(tes otc.{0,240}?(?:until|up to)[^0-9]{0,40}([0-9]{1,2})\.([0-9]{2})\s*(a\.m\.|p\.m\.))","TES OTC/LIS and Xetra EnLight"),"TES OTC, TES LIS, Xetra EnLight"});
    events.push_back({VenueSessionPhaseEventKind::PostTradingEnd,
        extract_time(R"(end of post-trading[^0-9]{0,40}([0-9]{1,2})\.([0-9]{2})\s*(a\.m\.|p\.m\.))","post-trading"),{}});
    std::sort(events.begin(),events.end(),[](const auto& a,const auto& b) {
        return std::tie(a.at.hour,a.at.minute,a.at.second)<std::tie(b.at.hour,b.at.minute,b.at.second);
    });
    validate_notice_phase_events(events);
    notice.kind=VenueSessionNoticeKind::SessionOverride;
    notice.day_status=VenueDayStatus::Trading;
    notice.product_sessions.clear();
    notice.phase_events=std::move(events);
    notice.cancels_notice_id.clear();
    notice.cancels_revision=0U;
    return notice;
}

VenueSessionNotice parse_xetra_session_notice_html(std::string_view html,VenueSessionNotice notice) {
    if(html.empty() || html.size()>4U*1024U*1024U)
        throw std::invalid_argument("Komunikat Xetra jest pusty lub przekracza limit 4 MiB.");
    static const std::regex id_pattern(R"(^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$)");
    static const std::regex source_id_pattern(R"(^sha256:[0-9a-f]{64}$)");
    if(!std::regex_match(notice.notice_id,id_pattern) || notice.revision==0U ||
       !std::regex_match(notice.calendar_id,id_pattern) || normalized(notice.venue)!="xetra" ||
       !std::regex_match(notice.source_object_id,source_id_pattern) ||
       notice.source_uri.rfind("https://",0)!=0 || notice.attribution.empty())
        throw std::invalid_argument("Metadane komunikatu Xetra są niekompletne lub nieprawidłowe.");
    auto parsed=std::unique_ptr<xmlDoc,decltype(&xmlFreeDoc)>(
        htmlReadMemory(html.data(),static_cast<int>(html.size()),"xetra-session-notice.html",nullptr,
            HTML_PARSE_NONET|HTML_PARSE_NOERROR|HTML_PARSE_NOWARNING|HTML_PARSE_RECOVER),xmlFreeDoc);
    if(!parsed) throw std::invalid_argument("Nie można sparsować HTML komunikatu Xetra.");

    std::vector<xmlNode*> paragraphs;
    collect_elements(xmlDocGetRootElement(parsed.get()),"p",paragraphs);
    std::optional<std::string> venue_text,date_text,kind_text,status_text,cancellation_text;
    const auto assign_once=[](std::optional<std::string>& target,std::string value,const char* field) {
        if(target) throw std::invalid_argument(std::string("Powtórzone pole komunikatu Xetra: ")+field);
        target=std::move(value);
    };
    const std::regex field_pattern(R"(^([a-z ]+):\s*(.+)$)");
    for(xmlNode* paragraph:paragraphs) {
        const auto text=normalized(node_text(paragraph));
        std::smatch match;
        if(!std::regex_match(text,match,field_pattern)) continue;
        const auto field=match[1].str();
        const auto value=match[2].str();
        if(field=="venue") assign_once(venue_text,value,"venue");
        else if(field=="effective date") assign_once(date_text,value,"effective date");
        else if(field=="notice kind") assign_once(kind_text,value,"notice kind");
        else if(field=="day status") assign_once(status_text,value,"day status");
        else if(field=="cancels") assign_once(cancellation_text,value,"cancels");
    }
    if(!venue_text || *venue_text!=normalized(notice.venue) || !date_text || !kind_text)
        throw std::invalid_argument("Komunikat Xetra musi jednoznacznie wskazywać rynek, datę i rodzaj.");
    if(!std::regex_match(*date_text,std::regex(R"(^[0-9]{4}-[0-9]{2}-[0-9]{2}$)")))
        throw std::invalid_argument("Data komunikatu Xetra musi mieć format YYYY-MM-DD.");
    notice.effective_date=parse_date(*date_text);

    std::vector<xmlNode*> tables;
    collect_elements(xmlDocGetRootElement(parsed.get()),"table",tables);
    std::vector<VenueProductSession> sessions;
    std::set<std::string> seen_product_groups;
    bool found_session_table=false;
    for(xmlNode* table:tables) {
        std::vector<xmlNode*> rows;
        collect_elements(table->children,"tr",rows);
        if(rows.empty()) continue;
        const auto header=direct_cells(rows.front());
        if(header.size()!=3U || normalized(node_text(header[0]))!="product group" ||
           normalized(node_text(header[1]))!="opens" || normalized(node_text(header[2]))!="closes")
            continue;
        if(found_session_table) throw std::invalid_argument("Komunikat Xetra zawiera kilka tabel sesji.");
        found_session_table=true;
        for(std::size_t row_index=1;row_index<rows.size();++row_index) {
            const auto cells=direct_cells(rows[row_index]);
            if(cells.size()!=3U) throw std::invalid_argument("Wiersz sesji Xetra musi zawierać grupę produktu, otwarcie i zamknięcie.");
            const auto product=normalized(node_text(cells[0]));
            if(product!="xetra_regular" && product!="shares_and_etps" && product!="structured_products")
                throw std::invalid_argument("Komunikat Xetra wskazuje nieobsługiwaną grupę produktów.");
            if(!seen_product_groups.insert(product).second)
                throw std::invalid_argument("Komunikat Xetra powtarza grupę produktów.");
            const auto parse_time=[](const std::string& raw) {
                static const std::regex pattern(R"(^([0-9]{2}):([0-9]{2})(?::([0-9]{2}))?$)");
                std::smatch match;
                const auto value=normalized(raw);
                if(!std::regex_match(value,match,pattern))
                    throw std::invalid_argument("Godzina sesji w komunikacie Xetra musi mieć format HH:MM[:SS].");
                const unsigned hour=static_cast<unsigned>(std::stoul(match[1].str()));
                const unsigned minute=static_cast<unsigned>(std::stoul(match[2].str()));
                const unsigned second=match[3].matched?static_cast<unsigned>(std::stoul(match[3].str())):0U;
                if(hour>23U || minute>59U || second>59U)
                    throw std::invalid_argument("Godzina sesji w komunikacie Xetra wykracza poza zakres doby.");
                return LocalTimeOfDay{hour,minute,second};
            };
            const auto opens=parse_time(node_text(cells[1]));
            const auto closes=parse_time(node_text(cells[2]));
            if(std::tie(opens.hour,opens.minute,opens.second)>=std::tie(closes.hour,closes.minute,closes.second))
                throw std::invalid_argument("Godzina otwarcia sesji Xetra musi poprzedzać zamknięcie.");
            sessions.push_back({product,opens,closes});
        }
    }

    notice.day_status.reset();
    notice.product_sessions.clear();
    notice.cancels_notice_id.clear();
    notice.cancels_revision=0U;
    if(*kind_text=="informational") {
        if(status_text || cancellation_text || !sessions.empty())
            throw std::invalid_argument("Komunikat informacyjny nie może zawierać nadpisania ani anulowania.");
        notice.kind=VenueSessionNoticeKind::Informational;
    } else if(*kind_text=="session override") {
        if(!status_text || cancellation_text)
            throw std::invalid_argument("Nadpisanie sesji wymaga statusu dnia i nie może anulować innego komunikatu.");
        if(*status_text=="trading") notice.day_status=VenueDayStatus::Trading;
        else if(*status_text=="closed") notice.day_status=VenueDayStatus::Closed;
        else if(*status_text=="settlement only") notice.day_status=VenueDayStatus::SettlementOnly;
        else throw std::invalid_argument("Nieznany status dnia w komunikacie Xetra.");
        if((*notice.day_status==VenueDayStatus::Trading && sessions.empty()) ||
           (*notice.day_status!=VenueDayStatus::Trading && !sessions.empty()))
            throw std::invalid_argument("Nadpisanie sesji wymaga kompletnych godzin tylko dla dnia handlowego.");
        notice.kind=VenueSessionNoticeKind::SessionOverride;
        notice.product_sessions=std::move(sessions);
    } else if(*kind_text=="cancellation") {
        if(!cancellation_text || status_text || !sessions.empty())
            throw std::invalid_argument("Anulowanie wymaga celu i nie może zawierać godzin ani statusu dnia.");
        const auto separator=cancellation_text->rfind('@');
        if(separator==std::string::npos || separator==0U || separator+1U>=cancellation_text->size())
            throw std::invalid_argument("Cel anulowania musi mieć format notice-id@revision.");
        notice.cancels_notice_id=cancellation_text->substr(0,separator);
        if(!std::regex_match(notice.cancels_notice_id,id_pattern))
            throw std::invalid_argument("Identyfikator anulowanego komunikatu jest nieprawidłowy.");
        const auto revision_text=cancellation_text->substr(separator+1U);
        if(!std::all_of(revision_text.begin(),revision_text.end(),[](unsigned char ch){return std::isdigit(ch)!=0;}))
            throw std::invalid_argument("Numer anulowanej rewizji jest nieprawidłowy.");
        const auto revision_value=std::stoul(revision_text);
        if(revision_value==0U || revision_value>std::numeric_limits<std::uint32_t>::max())
            throw std::invalid_argument("Numer anulowanej rewizji jest poza zakresem.");
        notice.cancels_revision=static_cast<std::uint32_t>(revision_value);
        notice.kind=VenueSessionNoticeKind::Cancellation;
    } else {
        throw std::invalid_argument("Nieznany rodzaj komunikatu Xetra.");
    }
    return notice;
}

} // namespace monitor::market
