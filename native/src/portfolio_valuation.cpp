#include "monitor/portfolio_valuation.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <set>
#include <stdexcept>
#include <tuple>

namespace monitor::market {
namespace {
bool currency_code(const std::string& value) {
    if(value.size()!=3U || value=="XAU") return false;
    for(const char c:value) if(c<'A'||c>'Z') return false;
    return true;
}
bool better(const MarketObservation& a,const MarketObservation& b) {
    if(a.observed_at.unix_seconds!=b.observed_at.unix_seconds)
        return a.observed_at.unix_seconds>b.observed_at.unix_seconds;
    return a.received_at.unix_seconds>b.received_at.unix_seconds;
}
struct Edge {
    std::string to;
    double rate{};
    const MarketObservation* observation{};
    std::string instrument_id;
};
struct Route {
    double rate{1.0};
    std::vector<const Edge*> edges;
};
std::optional<Route> find_route(const std::string& from,const std::string& to,
                                const std::map<std::string,std::vector<Edge>>& graph) {
    if(from==to) return Route{};
    struct Node { std::string currency; Route route; std::set<std::string> visited; };
    std::queue<Node> pending;
    pending.push({from,Route{},std::set<std::string>{from}});
    while(!pending.empty()) {
        auto node=std::move(pending.front());pending.pop();
        const auto it=graph.find(node.currency);
        if(it==graph.end()) continue;
        for(const auto& edge:it->second) {
            if(node.visited.contains(edge.to)) continue;
            Route next=node.route;
            next.rate*=edge.rate;
            if(!std::isfinite(next.rate)||next.rate<=0.0)
                throw std::overflow_error("Kurs krzyżowy FX przekracza zakres liczbowy.");
            next.edges.push_back(&edge);
            if(edge.to==to) return next;
            auto visited=node.visited;visited.insert(edge.to);
            pending.push({edge.to,std::move(next),std::move(visited)});
        }
    }
    return std::nullopt;
}
void add_provenance(PortfolioPriceProvenance& p,const MarketObservation& o,
                    const std::string& instrument_id) {
    p.instrument_ids.push_back(instrument_id);
    p.provider_ids.push_back(o.provider_id);
    if(o.source_uri) p.source_uris.push_back(*o.source_uri);
    p.price_kinds.push_back(o.price_kind);
    p.quality_flags.insert(p.quality_flags.end(),o.quality_flags.begin(),o.quality_flags.end());
    if(p.instrument_ids.size()==1U) {
        p.oldest_observation_unix=o.observed_at.unix_seconds;
        p.newest_observation_unix=o.observed_at.unix_seconds;
        p.oldest_received_unix=o.received_at.unix_seconds;
    } else {
        p.oldest_observation_unix=std::min(p.oldest_observation_unix,o.observed_at.unix_seconds);
        p.newest_observation_unix=std::max(p.newest_observation_unix,o.observed_at.unix_seconds);
        p.oldest_received_unix=std::min(p.oldest_received_unix,o.received_at.unix_seconds);
    }
}
}

PortfolioPriceBook build_portfolio_price_book(const MarketDataSnapshot& snapshot,
    const std::string& reporting_currency,UtcTimestamp as_of,std::int64_t max_age_seconds) {
    if(!currency_code(reporting_currency)||as_of.unix_seconds<0||max_age_seconds<0)
        throw std::invalid_argument("Nieprawidłowe parametry wyceny portfela.");
    std::map<std::string,const Instrument*> instruments;
    for(const auto& instrument:snapshot.instruments) instruments[instrument.id]=&instrument;

    using ObservationKey=std::pair<std::string,PriceKind>;
    std::map<ObservationKey,const MarketObservation*> latest;
    for(const auto& observation:snapshot.observations) {
        if(observation.quality!=QualityStatus::Validated ||
           observation.observed_at.unix_seconds>as_of.unix_seconds) continue;
        const auto age=as_of.unix_seconds-observation.observed_at.unix_seconds;
        if(age>max_age_seconds) continue;
        const auto instrument=instruments.find(observation.instrument_id);
        if(instrument==instruments.end()) continue;
        const auto& item=*instrument->second;
        const bool fx=item.asset_class==AssetClass::Currency && item.base_currency.has_value();
        const bool gold=item.asset_class==AssetClass::PreciousMetal && item.symbol=="XAU";
        if(!fx&&!gold) continue;
        if(fx && observation.price_kind!=PriceKind::Bid &&
           observation.price_kind!=PriceKind::Ask &&
           observation.price_kind!=PriceKind::Mid &&
           observation.price_kind!=PriceKind::Average &&
           observation.price_kind!=PriceKind::Reference) continue;
        if(gold && observation.price_kind!=PriceKind::Spot &&
           observation.price_kind!=PriceKind::Reference &&
           observation.price_kind!=PriceKind::Average) continue;
        if(!std::isfinite(observation.value)||observation.value<=0.0) continue;
        const ObservationKey key{observation.instrument_id,observation.price_kind};
        const auto old=latest.find(key);
        if(old==latest.end()||better(observation,*old->second)) latest[key]=&observation;
    }

    std::map<std::string,std::vector<Edge>> graph;
    std::set<std::string> currencies{reporting_currency};
    for(const auto& [id,instrument_ptr]:instruments) {
        const auto& instrument=*instrument_ptr;
        if(instrument.asset_class!=AssetClass::Currency||!instrument.base_currency) continue;
        const auto& base=*instrument.base_currency;
        const auto& quote=instrument.quote_currency;
        if(!currency_code(base)||!currency_code(quote)||base==quote) continue;
        currencies.insert(base);currencies.insert(quote);
        auto get=[&](PriceKind kind)->const MarketObservation*{
            const auto it=latest.find({id,kind});return it==latest.end()?nullptr:it->second;
        };
        const MarketObservation* bid=get(PriceKind::Bid);
        const MarketObservation* ask=get(PriceKind::Ask);
        const MarketObservation* fallback=get(PriceKind::Mid);
        if(!fallback) fallback=get(PriceKind::Average);
        if(!fallback) fallback=get(PriceKind::Reference);
        const auto* direct=bid?bid:fallback;
        const auto* inverse=ask?ask:fallback;
        if(direct) graph[base].push_back({quote,direct->value,direct,id});
        if(inverse) graph[quote].push_back({base,1.0/inverse->value,inverse,id});
    }

    PortfolioPriceBook book;
    book.reporting_currency=reporting_currency;
    book.as_of=as_of;
    book.prices.emplace_back(reporting_currency,1.0);
    for(const auto& currency:currencies) {
        if(currency==reporting_currency) continue;
        const auto route=find_route(currency,reporting_currency,graph);
        if(!route) continue;
        PortfolioPriceProvenance provenance;
        provenance.asset_code=currency;
        provenance.price_per_unit_reporting=route->rate;
        for(const auto* edge:route->edges) add_provenance(provenance,*edge->observation,edge->instrument_id);
        book.prices.emplace_back(currency,route->rate);
        book.provenance.push_back(std::move(provenance));
    }

    const MarketObservation* selected_gold=nullptr;
    const Instrument* selected_gold_instrument=nullptr;
    for(const auto& [id,instrument_ptr]:instruments) {
        const auto& instrument=*instrument_ptr;
        if(instrument.asset_class!=AssetClass::PreciousMetal||instrument.symbol!="XAU") continue;
        for(const auto kind:{PriceKind::Spot,PriceKind::Reference,PriceKind::Average}) {
            const auto it=latest.find({id,kind});
            if(it==latest.end()) continue;
            if(!selected_gold||better(*it->second,*selected_gold)) {
                selected_gold=it->second;
                selected_gold_instrument=&instrument;
            }
        }
    }
    if(selected_gold&&selected_gold_instrument) {
        const double grams_per_unit=convert_quantity(1.0,selected_gold_instrument->quantity_unit,Unit::Gram);
        const double price_per_gram=selected_gold->value/grams_per_unit;
        const auto fx=find_route(selected_gold_instrument->quote_currency,reporting_currency,graph);
        if(fx) {
            const double price=price_per_gram*fx->rate;
            if(!std::isfinite(price)||price<=0.0)
                throw std::overflow_error("Cena złota w walucie raportowania przekracza zakres.");
            PortfolioPriceProvenance provenance;
            provenance.asset_code="XAU";
            provenance.price_per_unit_reporting=price;
            add_provenance(provenance,*selected_gold,selected_gold_instrument->id);
            for(const auto* edge:fx->edges) add_provenance(provenance,*edge->observation,edge->instrument_id);
            book.prices.emplace_back("XAU",price);
            book.provenance.push_back(std::move(provenance));
        }
    }
    std::sort(book.prices.begin(),book.prices.end(),[](const auto& a,const auto& b){return a.first<b.first;});
    std::sort(book.provenance.begin(),book.provenance.end(),[](const auto& a,const auto& b){return a.asset_code<b.asset_code;});
    return book;
}

} // namespace monitor::market
