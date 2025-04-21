#pragma once
#include "ExtendedCurve.h"
namespace ACHS {
	class ExtendedCurves {
	public:
		ExtendedCurves(Spread spread = 0.0001) {
			spread_up_ = Handle<Quote>(std::make_shared<SimpleQuote>(spread));
			spread_down_ = Handle<Quote>(std::make_shared<SimpleQuote>(-spread));
			bond_engine_ = std::make_shared<DiscountingBondEngine>(active_curve_);
		}

		template<typename Method>
		void addOrUpdate(
			const std::string& name,
			const std::shared_ptr<YieldTermStructure>& base,
			const Method& method) 
		{
			curve_map_[name] = std::make_shared<ExtendedCurveWrapper>(
				base,
				method);
		}

		void addOrUpdate(
			const std::string& name,
			const std::shared_ptr<ExtendedCurveWrapper>& wrapper) 
		{
			curve_map_[name] = wrapper;
		}

		void setActiveCurve(const std::string& name) 
		{
			auto it = curve_map_.find(name);
			if (it == curve_map_.end()) {
				throw std::runtime_error("Curve '" + name + "' not found");
			}

			active_curve_name_ = name;
			active_curve_.linkTo(it->second->curve());
			active_curve_up_ = std::make_shared<ZeroSpreadedTermStructure>(Handle<YieldTermStructure>(it->second->curve()), spread_up_);
			active_curve_down_ = std::make_shared<ZeroSpreadedTermStructure>(Handle<YieldTermStructure>(it->second->curve()), spread_down_);

		}

		std::shared_ptr<YieldTermStructure> activeCurve() const 
		{
			return *active_curve_;
		}
		std::string activeCurveName() const 
		{
			return active_curve_name_;
		}

		void linkToActiveCurve(const std::shared_ptr<Bond>& bond) const 
		{
			bond->setPricingEngine(bond_engine_);
		}

		Real NPV(const std::shared_ptr<Bond>& bond) const 
		{
			return bond->NPV();
		}

		Real NPV(const Leg& leg) const 
		{
			//std::cout << active_curve_->referenceDate() << "\n";
			return CashFlows::npv(leg, **active_curve_, false);
		}

		Real duration(const std::shared_ptr<Bond>& bond) {
			std::shared_ptr<YieldTermStructure> curve_base = *active_curve_;
			Real base = bond->NPV();
			active_curve_.linkTo(active_curve_up_);
			Real up = bond->NPV();
			active_curve_.linkTo(active_curve_down_);
			Real down = bond->NPV();
			active_curve_.linkTo(curve_base);

			return -(up - down) / (2.0 * base * spread_up_->value());

		}

		Real duration(const Leg& leg) {
			Real base = CashFlows::npv(leg, **active_curve_, false);
			Real up = CashFlows::npv(leg, *active_curve_up_, false);
			Real down = CashFlows::npv(leg, *active_curve_down_, false);

			return -(up - down) / (2.0 * base * spread_up_->value());

		}


		Real convexity(const std::shared_ptr<Bond>& bond) {
			std::shared_ptr<YieldTermStructure> curve_base = *active_curve_;

			Real base = bond->NPV();
			active_curve_.linkTo(active_curve_up_);
			Real up = bond->NPV();
			active_curve_.linkTo(active_curve_down_);
			Real down = bond->NPV();
			active_curve_.linkTo(curve_base);

			return (up + down - 2.0 * base) / (base * spread_up_->value() * spread_up_->value());

		}

		Real convexity(const Leg& leg) {
			Real base = CashFlows::npv(leg, **active_curve_, false);
			Real up = CashFlows::npv(leg, *active_curve_up_, false);
			Real down = CashFlows::npv(leg, *active_curve_down_, false);

			return (up + down - 2.0 * base) / (base * spread_up_->value() * spread_up_->value());

		}

	private:
		std::map<std::string, std::shared_ptr<ExtendedCurveWrapper>> curve_map_;

		std::string active_curve_name_;
		RelinkableHandle<YieldTermStructure> active_curve_;

		Handle<Quote> spread_up_;
		Handle<Quote> spread_down_;
		std::shared_ptr<YieldTermStructure> active_curve_up_;
		std::shared_ptr<YieldTermStructure> active_curve_down_;

		std::shared_ptr<DiscountingBondEngine> bond_engine_;
	};
}