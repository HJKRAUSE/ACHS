#pragma once
#include <ql/quantlib.hpp>
namespace ACHS {
	using namespace QuantLib;
	template<typename Method>
	class ExtendedCurve {
	public:
		ExtendedCurve(
			const std::shared_ptr<YieldTermStructure>& base,
			const Method& method) { 
		
			base->enableExtrapolation();
			extended_curve_ = method.buildCurve(base);
		}

		std::shared_ptr<YieldTermStructure> curve() const { return extended_curve_; }

	private:
		std::shared_ptr<YieldTermStructure> extended_curve_;

	};

	class ExtendedCurveWrapper {
	public:
		template<typename Method>
		ExtendedCurveWrapper(
			const std::shared_ptr<YieldTermStructure>& base,
			const Method& method) {

			auto curve = std::make_shared<ExtendedCurve<Method>>(base, method);
			extended_curve_ = [curve]() { return curve->curve(); };

			holder_ = curve; // Force curve to remain alive
 			}
		std::shared_ptr<YieldTermStructure> curve() const { return extended_curve_(); }

	private:
		std::function<std::shared_ptr<YieldTermStructure>()> extended_curve_;

		std::shared_ptr<void> holder_;
	};
}