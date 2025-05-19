#pragma once
#include "ExtensionMethod.h"
#include <ql/quantlib.hpp>
#include <algorithm>
#include <vector>

namespace ACHS {

	using namespace QuantLib;

	template<typename Trait>
	class DualBlended : public ExtensionMethod<DualBlended<Trait>, Trait> {
		friend class ExtensionMethod<DualBlended<Trait>, Trait>;
	public:
		DualBlended(
			const Period& start_period,
			const Period& end_period,
			const Period& step = Period(1, Months),
			const Period& d1 = Period(20, Years),
			const Period& d2 = Period(30, Years)) :
			d1_(d1), d2_(d2),
			ExtensionMethod<DualBlended<Trait>, Trait>(start_period, end_period, step) {}

	protected:
		std::shared_ptr<YieldTermStructure> buildCurveImpl(
			const std::shared_ptr<YieldTermStructure>& base) const {

			DayCounter day_counter = base->dayCounter();

			Date reference_date = base->referenceDate();

			Date start_date = reference_date + this->start_period_;
			Date end_date = reference_date + this->end_period_;

			Time start_time = day_counter.yearFraction(reference_date, start_date);
			Time end_time = day_counter.yearFraction(reference_date, end_date);

			Time t1 = day_counter.yearFraction(reference_date, reference_date + d1_);
			Time t2 = day_counter.yearFraction(reference_date, reference_date + d2_);
			Rate r1 = extractRate<Trait>(base, t1);
			Rate r2 = extractRate<Trait>(base, t2);
			Rate ultimate = (r1 + r2) / 2.0;

			std::vector<Date> dates;
			std::vector<Rate> rates;

			for (Date d = reference_date; d <= end_date; d += this->step_) {
				Time t = day_counter.yearFraction(reference_date, d);
				Rate r = (t < start_time) ? extractRate<Trait>(base, t) : ultimate;

				dates.push_back(d);
				rates.push_back(r);
			}

			for (std::size_t i = 1; i < dates.size(); ++i) {
				if (dates[i] == dates[i - 1]) {
					std::cerr << "Duplicate date: " << dates[i] << std::endl;
				}
			}

			if constexpr (std::is_same_v<Trait, Traits::Zero>) {
				return std::make_shared<ZeroCurve>(dates, rates, day_counter);
			}
			else if constexpr (std::is_same_v<Trait, Traits::Forward>) {
				return std::make_shared<ForwardCurve>(dates, rates, day_counter);
			}
			else {
				throw std::runtime_error("DualBlended::buildCurveImpl: unknown trait.");
			}
		}
	private:
		Period d1_;
		Period d2_;
	};

}