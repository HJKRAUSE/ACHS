#include <ql/quantlib.hpp>
#include <map>
#include <fstream>
#include <iomanip>
#include <chrono>

// Helper to construct curves more easily
#include "TreasuryQuote.h"

// Extension logic
#include "ExtensionMethod.h"
#include "ExtendedCurve.h"
#include "ExtendedCurves.h"

// Extension methods
#include "Constant.h"
#include "Flat.h"
#include "LinearlyGraded.h"
#include "RollingAverage.h"
#include "DualBlended.h"

// LCF generation
#include "LiabilityCashFlows.h"


using namespace QuantLib;
using namespace ACHS;

std::shared_ptr<Bond> makeBond(const Date& today, const Period& tenor, Rate coupon = 0.05) {
    Calendar calendar = UnitedStates(UnitedStates::GovernmentBond);
    Date maturity = calendar.advance(today, tenor);
    Schedule schedule(today, maturity, Period(Semiannual), calendar,
        Unadjusted, Unadjusted, DateGeneration::Backward, false);
    return std::make_shared<FixedRateBond>(
        1, 100.0, schedule, std::vector<Rate>{coupon}, ActualActual(ActualActual::Bond));
}

int main() {
    std::cout << "ACHS Surplus Volatility\nHarold James Krause\n05-19-2025\n\n";

    Date today(31, Dec, 2024);

    Settings::instance().evaluationDate() = today;

    std::vector<TreasuryQuote> treasury_quotes = {
        TreasuryQuote(100.0000, 4.2275, Period(1, Months)),
        TreasuryQuote(100.0000, 4.2225, Period(2, Months)),
        TreasuryQuote(100.0000, 4.2150, Period(3, Months)),
        TreasuryQuote(100.0000, 4.2150, Period(4, Months)),
        TreasuryQuote(100.0000, 4.0575, Period(6, Months)),
        TreasuryQuote(100.0000, 3.8575, Period(1, Years)),
        TreasuryQuote(100.0898, 3.8750, Period(2, Years)),
        TreasuryQuote(099.7695, 3.7500, Period(3, Years)),
        TreasuryQuote(100.1953, 4.0000, Period(5, Years)),
        TreasuryQuote(100.0078, 4.1250, Period(7, Years)),
        TreasuryQuote(102.4688, 4.6250, Period(10, Years)),
        TreasuryQuote(100.0000, (4.6250 + 4.7500) / 2.0, Period(15, Years)),    //synthetic
        TreasuryQuote(099.3281, 4.7500, Period(20, Years)),
        TreasuryQuote(100.0000, (4.75 + 4.625) / 2.0, Period(25, Years)),       //synthetic
        TreasuryQuote(097.7500, 3.6250, Period(30, Years)) };                   // base quote is 4.625% par

    
    std::vector<std::shared_ptr<RateHelper>> rate_helpers;
    std::vector<std::shared_ptr<BondHelper>> bond_helpers;
    for (const TreasuryQuote& quote : treasury_quotes) {
        rate_helpers.push_back(quote.makeHelper<RateHelper>());
        bond_helpers.push_back(quote.makeHelper<BondHelper>());
    }

    DayCounter dc = ActualActual(ActualActual::Actual365);
    Calendar calendar = UnitedStates(UnitedStates::GovernmentBond);

    std::vector<std::pair<std::string, std::shared_ptr<YieldTermStructure>>> base_yield_curves{
        {"PIECEWISE_ZERO_LINEAR", std::make_shared<PiecewiseYieldCurve<ZeroYield, Linear>>(today, rate_helpers, dc)},
        {"PIECEWISE_DISCOUNT_LOGLINEAR", std::make_shared<PiecewiseYieldCurve<Discount, LogLinear>>(today, rate_helpers, dc)},
        {"PIECEWISE_ZERO_CUBIC", std::make_shared<PiecewiseYieldCurve<ZeroYield, Cubic>>(today, rate_helpers, dc)},
        {"FITTED_NELSON_SIEGEL", std::make_shared<FittedBondDiscountCurve>(0, calendar, bond_helpers, dc, NelsonSiegelFitting())},
        {"FITTED_NELSON_SIEGEL_SVENSSON", std::make_shared<FittedBondDiscountCurve>(0, calendar, bond_helpers, dc, SvenssonFitting())},
        {"FITTED_EXPONENTIAL_SPLINES", std::make_shared<FittedBondDiscountCurve>(0, calendar, bond_helpers, dc, ExponentialSplinesFitting())}
    };


    ExtendedCurves yield_curves(0.001);
    for (const std::pair<std::string, std::shared_ptr<YieldTermStructure>>& base : base_yield_curves) {
        // Forward-rate extensions
        yield_curves.addOrUpdate(base.first + ":FLAT_FORWARD", base.second, Flat<Traits::Forward>(Period(30, Years), Period(100, Years)));
        yield_curves.addOrUpdate(base.first + ":CONSTANT_FORWARD", base.second, Constant<Traits::Forward>(Rate(0.05), Period(30, Years), Period(100, Years)));
        yield_curves.addOrUpdate(base.first + ":LINEARLY_GRADED_FORWARD", base.second, LinearlyGraded<Traits::Forward>(Rate(0.05), Period(30, Years), Period(40, Years), Period(100, Years)));
        yield_curves.addOrUpdate(base.first + ":ROLLING_AVERAGE_FORWARD", base.second, RollingAverage<Traits::Forward>(60, Period(30, Years), Period(100, Years)));
        yield_curves.addOrUpdate(base.first + ":DUAL_BLENDED_FORWARD", base.second, DualBlended<Traits::Forward>(Period(30, Years), Period(100, Years)));
        // Zero-rate extensions
        yield_curves.addOrUpdate(base.first + ":FLAT_ZERO", base.second, Flat<Traits::Zero>(Period(30, Years), Period(100, Years)));
        yield_curves.addOrUpdate(base.first + ":CONSTANT_ZERO", base.second, Constant<Traits::Zero>(Rate(0.05), Period(30, Years), Period(100, Years)));
        yield_curves.addOrUpdate(base.first + ":LINEARLY_GRADED_ZERO", base.second, LinearlyGraded<Traits::Zero>(Rate(0.05), Period(30, Years), Period(40, Years), Period(100, Years)));
        yield_curves.addOrUpdate(base.first + ":ROLLING_AVERAGE_ZERO", base.second, RollingAverage<Traits::Zero>(60, Period(30, Years), Period(100, Years)));
        yield_curves.addOrUpdate(base.first + ":DUAL_BLENDED_ZERO", base.second, DualBlended<Traits::Forward>(Period(30, Years), Period(100, Years)));
    }
    

    std::vector<std::string> yield_curve_names{
        "PIECEWISE_ZERO_LINEAR:FLAT_FORWARD",
        "PIECEWISE_ZERO_LINEAR:CONSTANT_FORWARD",
        "PIECEWISE_ZERO_LINEAR:LINEARLY_GRADED_FORWARD",
        "PIECEWISE_ZERO_LINEAR:ROLLING_AVERAGE_FORWARD",
        "PIECEWISE_ZERO_LINEAR:DUAL_BLENDED_FORWARD",

        "PIECEWISE_ZERO_LINEAR:FLAT_ZERO",
        "PIECEWISE_ZERO_LINEAR:CONSTANT_ZERO",
        "PIECEWISE_ZERO_LINEAR:LINEARLY_GRADED_ZERO",
        "PIECEWISE_ZERO_LINEAR:ROLLING_AVERAGE_ZERO",
        "PIECEWISE_ZERO_LINEAR:DUAL_BLENDED_ZERO",

        "PIECEWISE_DISCOUNT_LOGLINEAR:FLAT_FORWARD",
        "PIECEWISE_DISCOUNT_LOGLINEAR:CONSTANT_FORWARD",
        "PIECEWISE_DISCOUNT_LOGLINEAR:LINEARLY_GRADED_FORWARD",
        "PIECEWISE_DISCOUNT_LOGLINEAR:ROLLING_AVERAGE_FORWARD",
        "PIECEWISE_DISCOUNT_LOGLINEAR:DUAL_BLENDED_FORWARD",

        "PIECEWISE_DISCOUNT_LOGLINEAR:FLAT_ZERO",
        "PIECEWISE_DISCOUNT_LOGLINEAR:CONSTANT_ZERO",
        "PIECEWISE_DISCOUNT_LOGLINEAR:LINEARLY_GRADED_ZERO",
        "PIECEWISE_DISCOUNT_LOGLINEAR:ROLLING_AVERAGE_ZERO",
        "PIECEWISE_DISCOUNT_LOGLINEAR:DUAL_BLENDED_ZERO",

        "PIECEWISE_ZERO_CUBIC:FLAT_FORWARD",
        "PIECEWISE_ZERO_CUBIC:CONSTANT_FORWARD",
        "PIECEWISE_ZERO_CUBIC:LINEARLY_GRADED_FORWARD",
        "PIECEWISE_ZERO_CUBIC:ROLLING_AVERAGE_FORWARD",
        "PIECEWISE_ZERO_CUBIC:DUAL_BLENDED_FORWARD",

        "PIECEWISE_ZERO_CUBIC:FLAT_ZERO",
        "PIECEWISE_ZERO_CUBIC:CONSTANT_ZERO",
        "PIECEWISE_ZERO_CUBIC:LINEARLY_GRADED_ZERO",
        "PIECEWISE_ZERO_CUBIC:ROLLING_AVERAGE_ZERO",
        "PIECEWISE_ZERO_CUBIC:DUAL_BLENDED_ZERO",

        "FITTED_NELSON_SIEGEL:FLAT_FORWARD",
        "FITTED_NELSON_SIEGEL:CONSTANT_FORWARD",
        "FITTED_NELSON_SIEGEL:LINEARLY_GRADED_FORWARD",
        "FITTED_NELSON_SIEGEL:ROLLING_AVERAGE_FORWARD",
        "FITTED_NELSON_SIEGEL:DUAL_BLENDED_FORWARD",

        "FITTED_NELSON_SIEGEL:FLAT_ZERO",
        "FITTED_NELSON_SIEGEL:CONSTANT_ZERO",
        "FITTED_NELSON_SIEGEL:LINEARLY_GRADED_ZERO",
        "FITTED_NELSON_SIEGEL:ROLLING_AVERAGE_ZERO",
        "FITTED_NELSON_SIEGEL:DUAL_BLENDED_ZERO",

        "FITTED_NELSON_SIEGEL_SVENSSON:FLAT_FORWARD",
        "FITTED_NELSON_SIEGEL_SVENSSON:CONSTANT_FORWARD",
        "FITTED_NELSON_SIEGEL_SVENSSON:LINEARLY_GRADED_FORWARD",
        "FITTED_NELSON_SIEGEL_SVENSSON:ROLLING_AVERAGE_FORWARD",
        "FITTED_NELSON_SIEGEL_SVENSSON:DUAL_BLENDED_FORWARD",

        "FITTED_NELSON_SIEGEL_SVENSSON:FLAT_ZERO",
        "FITTED_NELSON_SIEGEL_SVENSSON:CONSTANT_ZERO",
        "FITTED_NELSON_SIEGEL_SVENSSON:LINEARLY_GRADED_ZERO",
        "FITTED_NELSON_SIEGEL_SVENSSON:ROLLING_AVERAGE_ZERO",
        "FITTED_NELSON_SIEGEL_SVENSSON:DUAL_BLENDED_ZERO",

        "FITTED_EXPONENTIAL_SPLINES:FLAT_FORWARD",
        "FITTED_EXPONENTIAL_SPLINES:CONSTANT_FORWARD",
        "FITTED_EXPONENTIAL_SPLINES:LINEARLY_GRADED_FORWARD",
        "FITTED_EXPONENTIAL_SPLINES:ROLLING_AVERAGE_FORWARD",
        "FITTED_EXPONENTIAL_SPLINES:DUAL_BLENDED_FORWARD",

        "FITTED_EXPONENTIAL_SPLINES:FLAT_ZERO",
        "FITTED_EXPONENTIAL_SPLINES:CONSTANT_ZERO",
        "FITTED_EXPONENTIAL_SPLINES:LINEARLY_GRADED_ZERO",
        "FITTED_EXPONENTIAL_SPLINES:ROLLING_AVERAGE_ZERO",
        "FITTED_EXPONENTIAL_SPLINES:DUAL_BLENDED_ZERO"
    };

    std::ofstream curve_out("yield_curves.csv");
    curve_out << "CurveName,Date,ForwardRate\n";

    for (const auto& name : yield_curve_names) {
        yield_curves.setActiveCurve(name);
        auto curve = yield_curves.activeCurve();
        Date ref = today;

        for (int m = 1; m <= 70 * 12; ++m) {
            Date start = ref + Period(m - 1, Months);
            Date end = ref + Period(m, Months);
            try {
                Rate fwd = curve->forwardRate(start, end, dc, Continuous).rate();
                curve_out << name << "," << io::iso_date(end) << "," << std::fixed << std::setprecision(6) << fwd << "\n";
            }
            catch (...) {
                // Protect from silly extrapolation errors
            }
        }
    }
    curve_out.close();

    LiabilityCashFlows liability_cash_flows("liability_cash_flows.csv");
    std::ofstream liab_out("liabilities_30y_down_100bps.csv");
    liab_out << "CurveName,NPV,Duration,Convexity\n";

    std::ofstream bond_out("assets_30y_down_100bps.csv");
    bond_out << "CurveName,Tenor,NPV,Duration,Convexity\n";

    std::vector<Period> tenors = { Period(5, Years), Period(10, Years), Period(20, Years), Period(30, Years) };

    for (const auto& name : yield_curve_names) {
        yield_curves.setActiveCurve(name);
        for (const auto& tenor : tenors) {
            auto bond = makeBond(today, tenor);
            yield_curves.linkToActiveCurve(bond);
            Real npv = yield_curves.NPV(bond);
            Real dur = yield_curves.duration(bond);
            Real con = yield_curves.convexity(bond);
            bond_out << name << "," << tenor.length() << " " << tenor.units() << ","
                << std::fixed << std::setprecision(6) << npv << "," << dur << "," << con << "\n";
        }
        Real npv = yield_curves.NPV(liability_cash_flows.leg());
        Real dur = yield_curves.duration(liability_cash_flows.leg());
        Real con = yield_curves.convexity(liability_cash_flows.leg());
        liab_out << name << "," << std::fixed << std::setprecision(6) << npv << "," << dur << "," << con << "\n";
    }
    bond_out.close();
    liab_out.close();

    return 0;
}