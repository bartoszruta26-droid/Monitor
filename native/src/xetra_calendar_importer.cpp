#include "monitor/market_data_calendar.hpp"

#include <libxml/HTMLparser.h>
#include <libxml/tree.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <map>
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
    if(year!=expected_year)
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

CivilDate date_from_cell(const std::string& text, int expected_year) {
    std::smatch match;
    if(!std::regex_search(text,match,year_date_pattern()))
        throw std::invalid_argument("Brak jednoznacznej daty z rokiem w komórce tabeli Xetra.");
    return parse_year_date(match,expected_year);
}

std::vector<CivilDate> dates_in_text(const std::string& text, int expected_year) {
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
    bool found_2026_table = false;
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
            for (std::size_t i = 0; i < cells.size(); ++i) {
                if(normalized(node_text(cells[i]))==std::to_string(calendar_year)) year_column=i;
            }
            if (year_column == static_cast<std::size_t>(-1)) continue;
            found_2026_table = true;
            if (cells.size() <= year_column || cells.size() < 2U) continue;
            const std::string label = normalized(node_text(cells.front()));
            if (!required.contains(label)) continue;
            const auto date=date_from_cell(node_text(cells[year_column]),calendar_year);
            if (!holidays.emplace(label, date).second)
                throw std::invalid_argument("Powtórzony wiersz święta w tabeli Xetra HTML.");
        }
        if (found_2026_table && holidays.size() == required.size()) break;
    }
    if (!found_2026_table || holidays.size() != required.size())
        throw std::invalid_argument("Tabela Xetra HTML nie zawiera kompletnego zestawu ośmiu wierszy dla 2026.");

    const std::map<std::string, std::string> expected{
        {"new year's day","2026-01-01"},{"good friday","2026-04-03"},
        {"easter monday","2026-04-06"},{"labor day","2026-05-01"},
        {"christmas eve","2026-12-24"},{"christmas day","2026-12-25"},
        {"boxing day","2026-12-26"},{"new year's eve","2026-12-31"}
    };
    for (const auto& [label, date] : holidays) {
        if (format_date(date) != expected.at(label))
            throw std::invalid_argument("Data święta Xetra 2026 nie zgadza się z oczekiwanym kalendarzem.");
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
        throw std::invalid_argument("Brak semantycznej informacji o handlu w święta 2026.");
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
            const auto parsed_dates=dates_in_text(node_text(item),calendar_year);
            if(parsed_dates.size()!=1U ||
               !actual_trading.emplace(name,parsed_dates.front()).second ||
               !trading_dates.insert(format_date(parsed_dates.front())).second)
                throw std::invalid_argument("Data święta z handlem jest niejednoznaczna lub powtórzona.");
        }
    }
    if(actual_trading.size()!=named_trading.size())
        throw std::invalid_argument("Lista świąt z handlem Xetra jest niekompletna lub nieznana.");

    SourceCalendarDocument source = document;
    std::set<std::string> extraordinary_notice_dates;
    for(xmlNode* paragraph:paragraphs) {
        const auto label=normalized(node_text(paragraph));
        if(label.find("deviating trading hours may apply")==std::string::npos) continue;
        const auto dates=dates_in_text(node_text(paragraph),calendar_year);
        if(dates.size()!=1U || !extraordinary_notice_dates.insert(format_date(dates.front())).second)
            throw std::invalid_argument("Komunikat o nadzwyczajnych godzinach Xetra ma niejednoznaczną datę.");
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

} // namespace monitor::market
