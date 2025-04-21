#pragma once
#include "ExtensionMethod.h"
#include <ql/quantlib.hpp>
#include <algorithm>
#include <vector>
namespace ACHS {

	using namespace QuantLib;

	template<typename Trait>
	class LinearlyGraded : public ExtensionMethod<LinearlyGraded<Trait>, Trait> {
		friend class ExtensionMethod<LinearlyGraded<Trait>, Trait>;
	public:
		LinearlyGraded(
			Rate ultimate_rate,
			const Period& start_period,
			const Period& grading_end_period,
			const Period& end_period,
			const Period& step = Period(1, Months)) :
			ultimate_rate_(ultimate_rate), grading_end_period_(grading_end_period),
			ExtensionMethod<LinearlyGraded<Trait>, Trait>(start_period, end_period, step) {}

	protected:
		std::shared_ptr<YieldTermStructure> buildCurveImpl(
			const std::shared_ptr<YieldTermStructure>& base) const {

			DayCounter day_counter = base->dayCounter();

			Date reference_date = base->referenceDate();

			Date start_date = reference_date + this->start_period_;
			Date grading_end_date = reference_date + grading_end_period_;
			Date end_date = reference_date + this->end_period_;

			Time start_time = day_counter.yearFraction(reference_date, start_date);
			Time grading_end_time = day_counter.yearFraction(reference_date, grading_end_date);
			Time end_time = day_counter.yearFraction(reference_date, end_date);

			std::vector<Date> dates;
			std::vector<Rate> rates;

			for (Date d = reference_date; d <= end_date; d += this->step_) {
				Time t = day_counter.yearFraction(reference_date, d);
				Rate r;
				if (t < start_time) {
					r = extractRate<Trait>(base, t);
				}
				else if (t <= grading_end_time) {
					Rate r_start = extractRate<Trait>(base, start_time);
					Real w = std::clamp((t - start_time) / (grading_end_time - start_time), 0.0, 1.0);
					r = r_start * (1 - w) + ultimate_rate_ * w;
				}
				else {
					r = ultimate_rate_;
				}

				dates.push_back(d);
				rates.push_back(r);
			}

			if constexpr (std::is_same_v<Trait, Traits::Zero>) {
				return std::make_shared<ZeroCurve>(dates, rates, day_counter);
			}
			else if constexpr (std::is_same_v<Trait, Traits::Forward>) {
				return std::make_shared<ForwardCurve>(dates, rates, day_counter);
			}
			else {
				throw std::runtime_error("LinearlyGraded::buildCurveImpl: unknown trait.");
			}
		}
	private:
		Rate ultimate_rate_;
		Period grading_end_period_;

	};

}