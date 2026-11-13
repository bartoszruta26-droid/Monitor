#include "monitor/portfolio_service.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <string_view>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>


namespace monitor::market {
namespace {
bool valid_history_currency(const std::string& code) {
    if(code.size()!=3U) return false;
    return std::all_of(code.begin(),code.end(),[](char ch){return ch>='A'&&ch<='Z';});
}
std::pair<std::string,std::string> parse_history_pair(const std::string& pair) {
    const auto slash=pair.find('/');
    if(slash==std::string::npos || pair.find('/',slash+1)!=std::string::npos)
        throw std::invalid_argument("Para FX musi mieć format BASE/QUOTE.");
    const auto base=pair.substr(0,slash), quote=pair.substr(slash+1);
    if(!valid_history_currency(base)||!valid_history_currency(quote)||base==quote)
        throw std::invalid_argument("Nieprawidłowa para walutowa w filtrze historii.");
    return {base,quote};
}
}

PortfolioHistoryReport build_portfolio_history(
    const PortfolioSnapshot& portfolio,
    const PortfolioHistoryFilter& filter) {
    if(!valid_history_currency(portfolio.reporting_currency))
        throw std::invalid_argument("Nieprawidłowa waluta raportowania historii.");
    if(filter.asset_code && filter.asset_code->empty())
        throw std::invalid_argument("Filtr aktywa nie może być pusty.");
    if(filter.source_id && filter.source_id->empty())
        throw std::invalid_argument("Filtr źródła nie może być pusty.");
    if(filter.operation_id && filter.operation_id->empty())
        throw std::invalid_argument("Filtr operacji nie może być pusty.");
    if(filter.executed_from_unix && *filter.executed_from_unix<0)
        throw std::invalid_argument("Początek zakresu historii nie może być ujemny.");
    if(filter.executed_to_unix && *filter.executed_to_unix<0)
        throw std::invalid_argument("Koniec zakresu historii nie może być ujemny.");
    if(filter.executed_from_unix && filter.executed_to_unix &&
       *filter.executed_from_unix>*filter.executed_to_unix)
        throw std::invalid_argument("Zakres czasu historii jest odwrócony.");

    std::optional<std::pair<std::string,std::string>> requested_pair;
    if(filter.fx_pair) requested_pair=parse_history_pair(*filter.fx_pair);

    std::map<std::string,std::string> operation_pairs;
    for(const auto& tx:portfolio.transactions) {
        if(tx.operation_id.empty()) continue;
        auto& pair=operation_pairs[tx.operation_id];
        // First leg temporarily stores its code; second leg completes canonical pair.
        if(pair.empty()) pair=tx.asset_code;
        else if(pair.find('/')==std::string::npos && pair!=tx.asset_code) {
            const auto first=pair;
            pair=first+"/"+tx.asset_code;
        }
    }

    PortfolioHistoryReport report;
    report.reporting_currency=portfolio.reporting_currency;
    std::map<std::string,PortfolioAssetActivity> activity;
    std::set<std::string> audited_operations;
    for(const auto& tx:portfolio.transactions) {
        if(filter.asset_code && tx.asset_code!=*filter.asset_code) continue;
        if(filter.source_id && tx.source_id!=*filter.source_id) continue;
        if(filter.operation_id && tx.operation_id!=*filter.operation_id) continue;
        if(filter.executed_from_unix && tx.executed_at_unix<*filter.executed_from_unix) continue;
        if(filter.executed_to_unix && tx.executed_at_unix>*filter.executed_to_unix) continue;

        std::string pair;
        if(!tx.operation_id.empty()) {
            const auto found=operation_pairs.find(tx.operation_id);
            if(found!=operation_pairs.end() && found->second.find('/')!=std::string::npos)
                pair=found->second;
        }
        if(requested_pair) {
            if(pair.empty()) continue;
            const auto actual=parse_history_pair(pair);
            const bool same=(actual==*requested_pair) ||
                (actual.first==requested_pair->second && actual.second==requested_pair->first);
            if(!same) continue;
        }
        report.entries.push_back({tx,pair,tx.fx_audit
            ?PortfolioPriceEvidence::ExecutedWithQuoteAudit
            :PortfolioPriceEvidence::RecordedTransaction});
        auto& item=activity[tx.asset_code];
        item.asset_code=tx.asset_code;
        ++item.transaction_count;
        if(tx.side==TradeSide::Buy) item.bought_quantity+=tx.quantity;
        else item.sold_quantity+=tx.quantity;
        item.fees_reporting+=tx.fee_reporting;
        report.total_fees_reporting+=tx.fee_reporting;
        if(tx.fx_audit && !tx.operation_id.empty())
            audited_operations.insert(tx.operation_id);
    }
    std::stable_sort(report.entries.begin(),report.entries.end(),
        [](const auto& a,const auto& b) {
            if(a.transaction.executed_at_unix!=b.transaction.executed_at_unix)
                return a.transaction.executed_at_unix<b.transaction.executed_at_unix;
            if(!a.transaction.operation_id.empty() &&
               a.transaction.operation_id==b.transaction.operation_id &&
               a.transaction.side!=b.transaction.side)
                return a.transaction.side==TradeSide::Sell;
            return a.transaction.id<b.transaction.id;
        });
    for(auto& [code,item]:activity) {
        (void)code;
        if(!std::isfinite(item.bought_quantity)||!std::isfinite(item.sold_quantity)||
           !std::isfinite(item.fees_reporting))
            throw std::overflow_error("Podsumowanie historii przekracza zakres liczbowy.");
        report.activity.push_back(std::move(item));
    }
    if(!std::isfinite(report.total_fees_reporting))
        throw std::overflow_error("Suma opłat historii przekracza zakres liczbowy.");
    report.transaction_count=report.entries.size();
    report.audited_fx_execution_count=audited_operations.size();
    return report;
}

namespace {
struct HistoryCursor {
    std::uint64_t revision{};
    std::int64_t executed_at{};
    std::string transaction_id;
};

HistoryCursor parse_history_cursor(const std::string& cursor) {
    const auto first=cursor.find(':');
    const auto second=first==std::string::npos?first:cursor.find(':',first+1);
    const auto third=second==std::string::npos?second:cursor.find(':',second+1);
    if(first==std::string::npos || second==std::string::npos ||
       third==std::string::npos || cursor.substr(0,first)!="v1" ||
       third+1>=cursor.size())
        throw std::invalid_argument("Nieprawidłowy kursor historii API.");
    HistoryCursor parsed;
    const auto revision_text=std::string_view(cursor).substr(first+1,second-first-1);
    const auto time_text=std::string_view(cursor).substr(second+1,third-second-1);
    const auto revision_result=std::from_chars(revision_text.data(),
        revision_text.data()+revision_text.size(),parsed.revision);
    const auto time_result=std::from_chars(time_text.data(),
        time_text.data()+time_text.size(),parsed.executed_at);
    if(revision_result.ec!=std::errc{} ||
       revision_result.ptr!=revision_text.data()+revision_text.size() ||
       time_result.ec!=std::errc{} ||
       time_result.ptr!=time_text.data()+time_text.size() ||
       parsed.executed_at<0)
        throw std::invalid_argument("Nieprawidłowe dane kursora historii API.");
    parsed.transaction_id=cursor.substr(third+1);
    return parsed;
}

std::string make_history_cursor(std::uint64_t revision,
    const PortfolioHistoryEntry& entry) {
    return "v1:"+std::to_string(revision)+":"+
        std::to_string(entry.transaction.executed_at_unix)+":"+
        entry.transaction.id;
}
}

PortfolioHistoryPage get_portfolio_history_page(
    const PortfolioSnapshot& portfolio,
    const PortfolioHistoryPageRequest& request) {
    if(request.api_version!=portfolio_history_api_version)
        throw std::invalid_argument("Nieobsługiwana wersja API historii portfela.");
    if(request.page_size==0U || request.page_size>500U)
        throw std::invalid_argument("Rozmiar strony historii musi mieścić się w zakresie 1–500.");

    const auto report=build_portfolio_history(portfolio,request.filter);
    std::size_t start=0;
    if(request.cursor) {
        const auto cursor=parse_history_cursor(*request.cursor);
        if(cursor.revision!=portfolio.revision)
            throw std::invalid_argument("Kursor historii należy do innej rewizji portfela.");
        const auto found=std::find_if(report.entries.begin(),report.entries.end(),
            [&](const auto& entry) {
                return entry.transaction.id==cursor.transaction_id &&
                    entry.transaction.executed_at_unix==cursor.executed_at;
            });
        if(found==report.entries.end())
            throw std::invalid_argument("Kursor historii nie wskazuje wpisu w bieżącym filtrze.");
        start=static_cast<std::size_t>(std::distance(report.entries.begin(),found))+1U;
    }

    PortfolioHistoryPage page;
    page.snapshot_revision=portfolio.revision;
    page.total_matching=report.entries.size();
    const auto remaining=report.entries.size()-std::min(start,report.entries.size());
    const auto count=std::min(request.page_size,remaining);
    page.entries.insert(page.entries.end(),
        report.entries.begin()+static_cast<std::ptrdiff_t>(start),
        report.entries.begin()+static_cast<std::ptrdiff_t>(start+count));
    if(start+count<report.entries.size() && !page.entries.empty())
        page.next_cursor=make_history_cursor(portfolio.revision,page.entries.back());
    return page;
}

PortfolioValuationReport value_saved_portfolio(
    const JsonPortfolioRepository& portfolio_repository,
    const MarketDataSnapshot& market_data,
    UtcTimestamp as_of,
    std::int64_t max_age_seconds) {
    auto portfolio=portfolio_repository.load();
    auto price_book=build_portfolio_price_book(
        market_data,portfolio.reporting_currency,as_of,max_age_seconds);
    auto summary=calculate_portfolio(
        portfolio.reporting_currency,portfolio.transactions,price_book.prices);
    return {std::move(portfolio),std::move(price_book),std::move(summary)};
}

} // namespace monitor::market
