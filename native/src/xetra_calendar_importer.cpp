#include "monitor/market_data_calendar.hpp"

#include <libxml/HTMLparser.h>
#include <libxml/tree.h>

#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
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

CivilDate date_from_cell(const std::string& text) {
    static const std::regex date_pattern(
        R"((January|February|March|April|May|June|July|August|September|October|November|December|Jan|Feb|Mar|Apr|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+([0-9]{1,2})[,]?\s+2026)",
        std::regex::icase);
    std::smatch match;
    if (!std::regex_search(text, match, date_pattern))
        throw std::invalid_argument("Brak poprawnej daty 2026 w komórce tabeli Xetra.");
    const auto month = month_number(match[1].str());
    const auto day = static_cast<unsigned>(std::stoul(match[2].str()));
    const auto date = CivilDate{2026, month, day};
    if (!is_valid_date(date)) throw std::invalid_argument("Niepoprawna data w tabeli Xetra.");
    return date;
}

std::vector<CivilDate> dates_in_text(const std::string& text) {
    static const std::regex date_pattern(
        R"((January|February|March|April|May|June|July|August|September|October|November|December|Jan|Feb|Mar|Apr|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+([0-9]{1,2})[,]?\s+2026)",
        std::regex::icase);
    std::vector<CivilDate> result;
    for (std::sregex_iterator it(text.begin(), text.end(), date_pattern), end; it != end; ++it) {
        const auto month = month_number((*it)[1].str());
        const auto day = static_cast<unsigned>(std::stoul((*it)[2].str()));
        const CivilDate date{2026, month, day};
        if (!is_valid_date(date)) throw std::invalid_argument("Niepoprawna data wyjątku handlowego Xetra.");
        result.push_back(date);
    }
    return result;
}

} // namespace

VenueSessionCalendar parse_xetra_html_calendar(
    std::string_view html, const SourceCalendarDocument& document) {
    validate_source_calendar_document(document);
    if (html.empty() || html.size() > 16U * 1024U * 1024U)
        throw std::invalid_argument("Dokument HTML Xetra jest pusty lub przekracza limit 16 MiB.");
    if (document.venue != "XETRA" || document.timezone != "Europe/Berlin" ||
        format_date(document.valid_from) != "2026-01-01" ||
        format_date(document.valid_to_exclusive) != "2027-01-01")
        throw std::invalid_argument("Importer HTML w tej wersji obsługuje wyłącznie Xetra 2026.");

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
                if (normalized(node_text(cells[i])) == "2026") year_column = i;
            }
            if (year_column == static_cast<std::size_t>(-1)) continue;
            found_2026_table = true;
            if (cells.size() <= year_column || cells.size() < 2U) continue;
            const std::string label = normalized(node_text(cells.front()));
            if (!required.contains(label)) continue;
            const auto date = date_from_cell(node_text(cells[year_column]));
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

    const std::string all_text = node_text(xmlDocGetRootElement(parsed.get()));
    const std::string notice_marker = "in the year 2026 regular stock exchange trading takes place";
    const auto notice_position = normalized(all_text).find(notice_marker);
    if (notice_position == std::string::npos)
        throw std::invalid_argument("Brak semantycznej informacji o handlu w święta 2026.");
    const auto notice_dates = dates_in_text(all_text.substr(notice_position));
    const std::set<std::string> expected_trading{
        "2026-05-14", "2026-05-25", "2026-06-04"
    };
    std::set<std::string> actual_trading;
    for (const auto& date : notice_dates) actual_trading.insert(format_date(date));
    if (actual_trading != expected_trading)
        throw std::invalid_argument("Lista świąt z handlem Xetra 2026 jest niekompletna lub nieznana.");

    SourceCalendarDocument source = document;
    VenueSessionCalendar result;
    result.source = std::move(source);
    using namespace std::chrono;
    const sys_days begin{year{2026}/January/1};
    const sys_days end{year{2027}/January/1};
    for (auto date = begin; date < end; date += days{1}) {
        const year_month_day ymd{date};
        VenueCalendarDay day;
        day.date = CivilDate{static_cast<int>(ymd.year()),
            static_cast<unsigned>(ymd.month()), static_cast<unsigned>(ymd.day())};
        const auto key = format_date(day.date);
        const bool settlement = key == "2026-12-24" || key == "2026-12-31";
        const bool closed = std::any_of(holidays.begin(), holidays.end(), [&](const auto& entry) {
            return format_date(entry.second) == key && !settlement;
        });
        const unsigned weekday_number = weekday{date}.iso_encoding();
        if (weekday_number >= 6U || closed) {
            day.status = VenueDayStatus::Closed;
        } else if (settlement) {
            day.status = VenueDayStatus::SettlementOnly;
        } else {
            day.status = VenueDayStatus::Trading;
            if (expected_trading.contains(key)) {
                day.product_sessions.push_back({"shares_and_etps",{9,0,0},{20,0,0}});
                day.product_sessions.push_back({"structured_products",{9,0,0},{22,0,0}});
            } else {
                day.product_sessions.push_back({"xetra_regular",{9,0,0},{17,30,0}});
            }
            day.requires_official_notice = key == "2026-12-30";
        }
        result.days.push_back(std::move(day));
    }
    return result;
}

} // namespace monitor::market
