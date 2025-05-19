Yield curve extensions built using QuantLib for the Actuaries' Club of Hartford and Springfield. This code creates
an ExtendedYieldCurve wrapper about QuantLib's YieldTermStructure class, which takes several extension method
implementations and extends the wrapped curve to some future date using (by default) monthly interpolated timesteps.

The resulting, extended curve is coded as an interpolated zero or forward curve and functions as-usual.
