#pragma once
#include <ql/quantlib.hpp>
#include <algorithm>
#include <vector>
namespace ACHS {
	namespace Traits {
		struct Zero {};
		struct Forward {};
	}

	using namespace QuantLib;

	template<typename Trait>
	QuantLib::Rate extractRate(const  std::shared_ptr<YieldTermStructure>& curve, QuantLib::Time t);

	template<>
	Rate extractRate<Traits::Zero>(const std::shared_ptr<YieldTermStructure>& curve, QuantLib::Time t) {
		return curve->zeroRate(t, Compounded).rate();
	}

	template<>
	Rate extractRate<Traits::Forward>(const  std::shared_ptr<YieldTermStructure>& curve, QuantLib::Time t) {
		return curve->forwardRate(t, t + 1.0 / 365.0, Compounded).rate();
	}

	template<typename Derived, typename Trait>
	class ExtensionMethod {
	public:
		std::shared_ptr<YieldTermStructure> buildCurve(
			const std::shared_ptr<YieldTermStructure>& base) const
		{
			return static_cast<const Derived*>(this)->buildCurveImpl(base);
		}
	protected:
		Period start_period_;
		Period end_period_;
		Period step_;

		ExtensionMethod(
			const Period& start_period,
			const Period& end_period,
			const Period& step) :
			start_period_(start_period), end_period_(end_period), step_(step) {}

	};

}