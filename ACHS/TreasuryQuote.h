#pragma once
#include <ql/quantlib.hpp>

namespace ACHS {
	template<typename>
	inline constexpr bool always_false = false;

	using namespace QuantLib;

	class TreasuryQuote {

	public:
		TreasuryQuote(Real quote, Rate rate, const Period& tenor) : quote_(quote), rate_(rate), tenor_(tenor) {
			frequency_ = isDepositRate() ? Once : Semiannual;
		}
		Real quote() const {
			return quote_;
		}
		Rate rate() const { 
			return rate_; 
		}
		Period tenor() const { 
			return tenor_; 
		}
		bool isDepositRate() const {
			return tenor_ <= Period(1, Years);
		}

		template<typename HelperType> 
		std::shared_ptr<HelperType> makeHelper() const {
			if constexpr (std::is_same_v<HelperType, BondHelper>) {
				return std::static_pointer_cast<BondHelper>(makeBondHelperImpl());
			}
			else if constexpr (std::is_same_v<HelperType, RateHelper>) {
				return std::static_pointer_cast<RateHelper>(isDepositRate() ? makeRateHelperImpl() : makeBondHelperImpl());
			}
			else { 
				// always_false delays initialization at compilation and prevents throwing a compile error
				// if the type is supported
				static_assert(always_false<HelperType>, "TreasuryQuote::makeHelper: unsupported HelperType.");
			}

		}
	private:
		Real quote_;
		Rate rate_;
		Period tenor_;

		Natural settlement_days_ = 1;
		Calendar calendar_ = UnitedStates(UnitedStates::GovernmentBond);
		Frequency frequency_;
		DayCounter day_counter_ = ActualActual(ActualActual::Bond);
		DayCounter deposit_day_counter_ = Actual360();
		BusinessDayConvention convention_ = Unadjusted;
		DateGeneration::Rule rule_ = DateGeneration::Forward;
		bool end_of_month_ = false;

		std::shared_ptr<BondHelper> makeBondHelperImpl() const {
			Date reference_date = Settings::instance().evaluationDate();
			Date maturity_date = calendar_.advance(reference_date, tenor_);
			Rate coupon_rate = rate_ / 100.0;

			Schedule schedule(
				reference_date,
				maturity_date,
				Period(frequency_),
				calendar_,
				convention_,
				convention_,
				rule_,
				end_of_month_);
			
			return std::make_shared<FixedRateBondHelper>(
				Handle<Quote>(std::make_shared<SimpleQuote>(quote_)),
				settlement_days_,
				100.0,
				schedule,
				std::vector<Rate> { coupon_rate },
				day_counter_,
				convention_,
				100.0);
		}

		std::shared_ptr<RateHelper> makeRateHelperImpl() const {
			Rate deposit_rate = rate_ / 100.0;
			return std::make_shared<DepositRateHelper>(
				Handle<Quote>(std::make_shared<SimpleQuote>(deposit_rate)),
				tenor_,
				settlement_days_,
				calendar_,
				convention_,
				end_of_month_,
				deposit_day_counter_);
		}

	};

}