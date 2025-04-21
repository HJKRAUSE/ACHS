#pragma once
#include "ExtensionMethod.h"
#include <ql/quantlib.hpp>
#include <algorithm>
#include <vector>
#include <deque>
namespace ACHS {

	using namespace QuantLib;

	template<typename Trait>
	class RollingAverage : public ExtensionMethod<RollingAverage<Trait>, Trait> {
		friend class ExtensionMethod<RollingAverage<Trait>, Trait>;
	public:
		RollingAverage(
			const Size window_size,
			const Period& start_period,
			const Period& end_period,
			const Period& step = Period(1, Months)) : 
			window_size_(window_size),
			ExtensionMethod<RollingAverage<Trait>, Trait>(start_period, end_period, step) { }

	protected:
		std::shared_ptr<YieldTermStructure> buildCurveImpl(
			const std::shared_ptr<YieldTermStructure>& base) const {

			DayCounter day_counter = base->dayCounter();
			Date reference_date = base->referenceDate();

			Date start_date = reference_date + this->start_period_;
			Date end_date = reference_date + this->end_period_;

			std::vector<Date> dates;
			std::vector<Rate> rates;

			std::deque<Rate> trailing_window;

			for (Date d = reference_date; d <= end_date; d += this->step_) {
				Time t = day_counter.yearFraction(reference_date, d);
				Rate r;

				if (d < start_date) {
					r = extractRate<Trait>(base, t);
				}
				else {
					// Use rolling average of prior window_size_ rates
					if (trailing_window.size() < window_size_) {
						r = trailing_window.empty()
							? extractRate<Trait>(base, t)
							: std::accumulate(trailing_window.begin(), trailing_window.end(), 0.0) / trailing_window.size();
					}
					else {
						r = std::accumulate(trailing_window.begin(), trailing_window.end(), 0.0) / window_size_;
					}
				}

				// Append to rolling window
				trailing_window.push_back(r);
				if (trailing_window.size() > window_size_)
					trailing_window.pop_front();

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
				throw std::runtime_error("RollingAverage::buildCurveImpl: unknown trait.");
			}
		}

	private:
		Size window_size_;
	};


}