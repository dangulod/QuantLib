/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*!
 Copyright (C) 2007 Allen Kuo
 Copyright (C) 2015 Andres Hernandez

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*  This example shows how to fit a term structure to a set of bonds
    using four different fitting methodologies. Though fitting is most
    useful for large numbers of bonds with non-smooth yield tenor
    structures, for comparison purposes, relatively smooth bond yields
    are fit here and compared to known solutions (par coupons), or
    results generated from the bootstrap fitting method.
*/

#include <ql/quantlib.hpp>

#ifdef BOOST_MSVC
/* Uncomment the following lines to unmask floating-point
   exceptions. Warning: unpredictable results can arise...

   See http://www.wilmott.com/messageview.cfm?catid=10&threadid=9481
   Is there anyone with a definitive word about this?
*/
// #include <float.h>
// namespace { unsigned int u = _controlfp(_EM_INEXACT, _MCW_EM); }
#endif

#include <boost/timer.hpp>
#include <iostream>
#include <iomanip>
#include <boost/make_shared.hpp>

#define LENGTH(a) (sizeof(a)/sizeof(a[0]))

using namespace std;
using namespace QuantLib;

#if defined(QL_ENABLE_SESSIONS)
namespace QuantLib {
    Integer sessionId() { return 0; }
}
#endif

// par-rate approximation
Rate parRate(const YieldTermStructure& yts,
             const std::vector<Date>& dates,
             const DayCounter& resultDayCounter) {
    QL_REQUIRE(dates.size() >= 2, "at least two dates are required");
    Real sum = 0.0;
    Time dt;
    for (Size i=1; i<dates.size(); ++i) {
        dt = resultDayCounter.yearFraction(dates[i-1], dates[i]);
        QL_REQUIRE(dt>=0.0, "unsorted dates");
        sum += yts.discount(dates[i]) * dt;
    }
    Real result = yts.discount(dates.front()) - yts.discount(dates.back());
    return result/sum;
}

void printOutput(const std::string& tag,
                 const boost::shared_ptr<FittedBondDiscountCurve>& curve) {
    cout << tag << endl;
    cout << "reference date : "
         << curve->referenceDate()
         << endl;
    cout << "number of iterations : "
         << curve->fitResults().numberOfIterations()
         << endl
         << endl;
}

Handle<YieldTermStructure> getDiscountCurve(){


    /*********************
     ***  MARKET DATA  ***
     *********************/

    Calendar calendar = TARGET();
    Integer fixingDays = 0;
    Date todaysDate = Settings::instance().evaluationDate();
    Date settlementDate = calendar.advance(todaysDate, fixingDays, Days);

    std::cout << "Today: " << todaysDate.weekday()
              << ", " << todaysDate << std::endl;

    std::cout << "Settlement date: " << settlementDate.weekday()
              << ", " << settlementDate << std::endl;

    // deposits
    Rate d1wQuote=0.0382;
    Rate d1mQuote=0.0372;
    Rate d3mQuote=0.0363;
    Rate d6mQuote=0.0353;
    Rate d9mQuote=0.0348;
    Rate d1yQuote=0.0345;
    // FRAs
    Rate fra3x6Quote=0.037125;
    Rate fra6x9Quote=0.037125;
    Rate fra6x12Quote=0.037125;
    // futures
    Real fut1Quote=96.2875;
    Real fut2Quote=96.7875;
    Real fut3Quote=96.9875;
    Real fut4Quote=96.6875;
    Real fut5Quote=96.4875;
    Real fut6Quote=96.3875;
    Real fut7Quote=96.2875;
    Real fut8Quote=96.0875;
    // swaps
    Rate s2yQuote=0.037125;
    Rate s3yQuote=0.0398;
    Rate s5yQuote=0.0443;
    Rate s10yQuote=0.05165;
    Rate s15yQuote=0.055175;


    /********************
     ***    QUOTES    ***
     ********************/

    // SimpleQuote stores a value which can be manually changed;
    // other Quote subclasses could read the value from a database
    // or some kind of data feed.

    // deposits
    boost::shared_ptr<Quote> d1wRate(new SimpleQuote(d1wQuote));
    boost::shared_ptr<Quote> d1mRate(new SimpleQuote(d1mQuote));
    boost::shared_ptr<Quote> d3mRate(new SimpleQuote(d3mQuote));
    boost::shared_ptr<Quote> d6mRate(new SimpleQuote(d6mQuote));
    boost::shared_ptr<Quote> d9mRate(new SimpleQuote(d9mQuote));
    boost::shared_ptr<Quote> d1yRate(new SimpleQuote(d1yQuote));
    // FRAs
    boost::shared_ptr<Quote> fra3x6Rate(new SimpleQuote(fra3x6Quote));
    boost::shared_ptr<Quote> fra6x9Rate(new SimpleQuote(fra6x9Quote));
    boost::shared_ptr<Quote> fra6x12Rate(new SimpleQuote(fra6x12Quote));
    // futures
    boost::shared_ptr<Quote> fut1Price(new SimpleQuote(fut1Quote));
    boost::shared_ptr<Quote> fut2Price(new SimpleQuote(fut2Quote));
    boost::shared_ptr<Quote> fut3Price(new SimpleQuote(fut3Quote));
    boost::shared_ptr<Quote> fut4Price(new SimpleQuote(fut4Quote));
    boost::shared_ptr<Quote> fut5Price(new SimpleQuote(fut5Quote));
    boost::shared_ptr<Quote> fut6Price(new SimpleQuote(fut6Quote));
    boost::shared_ptr<Quote> fut7Price(new SimpleQuote(fut7Quote));
    boost::shared_ptr<Quote> fut8Price(new SimpleQuote(fut8Quote));
    // swaps
    boost::shared_ptr<Quote> s2yRate(new SimpleQuote(s2yQuote));
    boost::shared_ptr<Quote> s3yRate(new SimpleQuote(s3yQuote));
    boost::shared_ptr<Quote> s5yRate(new SimpleQuote(s5yQuote));
    boost::shared_ptr<Quote> s10yRate(new SimpleQuote(s10yQuote));
    boost::shared_ptr<Quote> s15yRate(new SimpleQuote(s15yQuote));


    /*********************
     ***  RATE HELPERS ***
     *********************/

    // RateHelpers are built from the above quotes together with
    // other instrument dependant infos.  Quotes are passed in
    // relinkable handles which could be relinked to some other
    // data source later.

    // deposits
    DayCounter depositDayCounter = Actual360();

    boost::shared_ptr<RateHelper> d1w(new DepositRateHelper(
        Handle<Quote>(d1wRate),
        1*Weeks, fixingDays,
        calendar, ModifiedFollowing,
        true, depositDayCounter));
    boost::shared_ptr<RateHelper> d1m(new DepositRateHelper(
        Handle<Quote>(d1mRate),
        1*Months, fixingDays,
        calendar, ModifiedFollowing,
        true, depositDayCounter));
    boost::shared_ptr<RateHelper> d3m(new DepositRateHelper(
        Handle<Quote>(d3mRate),
        3*Months, fixingDays,
        calendar, ModifiedFollowing,
        true, depositDayCounter));
    boost::shared_ptr<RateHelper> d6m(new DepositRateHelper(
        Handle<Quote>(d6mRate),
        6*Months, fixingDays,
        calendar, ModifiedFollowing,
        true, depositDayCounter));
    boost::shared_ptr<RateHelper> d9m(new DepositRateHelper(
        Handle<Quote>(d9mRate),
        9*Months, fixingDays,
        calendar, ModifiedFollowing,
        true, depositDayCounter));
    boost::shared_ptr<RateHelper> d1y(new DepositRateHelper(
        Handle<Quote>(d1yRate),
        1*Years, fixingDays,
        calendar, ModifiedFollowing,
        true, depositDayCounter));


    // setup FRAs
    boost::shared_ptr<RateHelper> fra3x6(new FraRateHelper(
        Handle<Quote>(fra3x6Rate),
        3, 6, fixingDays, calendar, ModifiedFollowing,
        true, depositDayCounter));
    boost::shared_ptr<RateHelper> fra6x9(new FraRateHelper(
        Handle<Quote>(fra6x9Rate),
        6, 9, fixingDays, calendar, ModifiedFollowing,
        true, depositDayCounter));
    boost::shared_ptr<RateHelper> fra6x12(new FraRateHelper(
        Handle<Quote>(fra6x12Rate),
        6, 12, fixingDays, calendar, ModifiedFollowing,
        true, depositDayCounter));

    // setup swaps
    Frequency swFixedLegFrequency = Annual;
    BusinessDayConvention swFixedLegConvention = Unadjusted;
    DayCounter swFixedLegDayCounter = Thirty360(Thirty360::European);
    boost::shared_ptr<IborIndex> swFloatingLegIndex(new Euribor6M);

    boost::shared_ptr<RateHelper> s2y(new SwapRateHelper(
        Handle<Quote>(s2yRate), 2*Years,
        calendar, swFixedLegFrequency,
        swFixedLegConvention, swFixedLegDayCounter,
        swFloatingLegIndex));
    boost::shared_ptr<RateHelper> s3y(new SwapRateHelper(
        Handle<Quote>(s3yRate), 3*Years,
        calendar, swFixedLegFrequency,
        swFixedLegConvention, swFixedLegDayCounter,
        swFloatingLegIndex));
    boost::shared_ptr<RateHelper> s5y(new SwapRateHelper(
        Handle<Quote>(s5yRate), 5*Years,
        calendar, swFixedLegFrequency,
        swFixedLegConvention, swFixedLegDayCounter,
        swFloatingLegIndex));
    boost::shared_ptr<RateHelper> s10y(new SwapRateHelper(
        Handle<Quote>(s10yRate), 10*Years,
        calendar, swFixedLegFrequency,
        swFixedLegConvention, swFixedLegDayCounter,
        swFloatingLegIndex));
    boost::shared_ptr<RateHelper> s15y(new SwapRateHelper(
        Handle<Quote>(s15yRate), 15*Years,
        calendar, swFixedLegFrequency,
        swFixedLegConvention, swFixedLegDayCounter,
        swFloatingLegIndex));


    /*********************
     **  CURVE BUILDING **
     *********************/

    // Any DayCounter would be fine.
    // ActualActual::ISDA ensures that 30 years is 30.0
    DayCounter termStructureDayCounter =
        ActualActual(ActualActual::ISDA);


    double tolerance = 1.0e-15;

    // A depo-FRA-swap curve
    std::vector<boost::shared_ptr<RateHelper> > depoFRASwapInstruments;
    depoFRASwapInstruments.push_back(d1w);
    depoFRASwapInstruments.push_back(d1m);
    depoFRASwapInstruments.push_back(d3m);
    depoFRASwapInstruments.push_back(fra3x6);
    depoFRASwapInstruments.push_back(fra6x9);
    depoFRASwapInstruments.push_back(fra6x12);
    depoFRASwapInstruments.push_back(s2y);
    depoFRASwapInstruments.push_back(s3y);
    depoFRASwapInstruments.push_back(s5y);
    depoFRASwapInstruments.push_back(s10y);
    depoFRASwapInstruments.push_back(s15y);
    return Handle<YieldTermStructure>(
        boost::make_shared<PiecewiseYieldCurve<Discount,LogLinear> >(
                                       0, calendar, depoFRASwapInstruments,
                                       termStructureDayCounter,
                                       tolerance));
}

int main(int, char* []) {

    try {

        boost::timer timer;

        const Size numberOfBonds = 15;
        Real cleanPrice[numberOfBonds];

        for (Size i=0; i<numberOfBonds; i++) {
            cleanPrice[i]=100.0;
        }

        std::vector< boost::shared_ptr<SimpleQuote> > quote;
        for (Size i=0; i<numberOfBonds; i++) {
            boost::shared_ptr<SimpleQuote> cp(new SimpleQuote(cleanPrice[i]));
            quote.push_back(cp);
        }

        RelinkableHandle<Quote> quoteHandle[numberOfBonds];
        for (Size i=0; i<numberOfBonds; i++) {
            quoteHandle[i].linkTo(quote[i]);
        }

        Integer lengths[] = { 2, 4, 6, 8, 10, 12, 14, 16,
                              18, 20, 22, 24, 26, 28, 30 };
        Real coupons[] = { 0.0200, 0.0225, 0.0250, 0.0275, 0.0300,
                           0.0325, 0.0350, 0.0375, 0.0400, 0.0425,
                           0.0450, 0.0475, 0.0500, 0.0525, 0.0550 };

        Frequency frequency = Annual;
        DayCounter dc = SimpleDayCounter();
        BusinessDayConvention accrualConvention = ModifiedFollowing;
        BusinessDayConvention convention = ModifiedFollowing;
        Real redemption = 100.0;

        Calendar calendar = TARGET();
        Date today = calendar.adjust(Date::todaysDate());
        Date origToday = today;
        Settings::instance().evaluationDate() = today;

        // changing bondSettlementDays=3 increases calculation
        // time of exponentialsplines fitting method
        Natural bondSettlementDays = 0;
        Natural curveSettlementDays = 0;

        Date bondSettlementDate = calendar.advance(today, bondSettlementDays*Days);

        cout << endl;
        cout << "Today's date: " << today << endl;
        cout << "Bonds' settlement date: " << bondSettlementDate << endl;
        cout << "Calculating fit for 15 bonds....." << endl << endl;

        std::vector<boost::shared_ptr<BondHelper> > instrumentsA;
        std::vector<boost::shared_ptr<RateHelper> > instrumentsB;

        for (Size j=0; j<LENGTH(lengths); j++) {

            Date maturity = calendar.advance(bondSettlementDate, lengths[j]*Years);

            Schedule schedule(bondSettlementDate, maturity, Period(frequency),
                              calendar, accrualConvention, accrualConvention,
                              DateGeneration::Backward, false);

            boost::shared_ptr<BondHelper> helperA(
                     new FixedRateBondHelper(quoteHandle[j],
                                             bondSettlementDays,
                                             100.0,
                                             schedule,
                                             std::vector<Rate>(1,coupons[j]),
                                             dc,
                                             convention,
                                             redemption));

            boost::shared_ptr<RateHelper> helperB(
                     new FixedRateBondHelper(quoteHandle[j],
                                             bondSettlementDays,
                                             100.0,
                                             schedule,
                                             std::vector<Rate>(1, coupons[j]),
                                             dc,
                                             convention,
                                             redemption));
            instrumentsA.push_back(helperA);
            instrumentsB.push_back(helperB);
        }


        bool constrainAtZero = true;
        Real tolerance = 1.0e-10;
        Size max = 5000;

        boost::shared_ptr<YieldTermStructure> ts0 (
              new PiecewiseYieldCurve<Discount,LogLinear>(curveSettlementDays,
                                                          calendar,
                                                          instrumentsB,
                                                          dc));

        ExponentialSplinesFitting exponentialSplines(constrainAtZero);

        boost::shared_ptr<FittedBondDiscountCurve> ts1 (
                  new FittedBondDiscountCurve(curveSettlementDays,
                                              calendar,
                                              instrumentsA,
                                              dc,
                                              exponentialSplines,
                                              tolerance,
                                              max));

        printOutput("(a) exponential splines", ts1);


        SimplePolynomialFitting simplePolynomial(3, constrainAtZero);

        boost::shared_ptr<FittedBondDiscountCurve> ts2 (
                    new FittedBondDiscountCurve(curveSettlementDays,
                                                calendar,
                                                instrumentsA,
                                                dc,
                                                simplePolynomial,
                                                tolerance,
                                                max));

        printOutput("(b) simple polynomial", ts2);


        NelsonSiegelFitting nelsonSiegel;

        boost::shared_ptr<FittedBondDiscountCurve> ts3 (
                        new FittedBondDiscountCurve(curveSettlementDays,
                                                    calendar,
                                                    instrumentsA,
                                                    dc,
                                                    nelsonSiegel,
                                                    tolerance,
                                                    max));

        printOutput("(c) Nelson-Siegel", ts3);


        // a cubic bspline curve with 11 knot points, implies
        // n=6 (constrained problem) basis functions

        Time knots[] =  { -30.0, -20.0,  0.0,  5.0, 10.0, 15.0,
                           20.0,  25.0, 30.0, 40.0, 50.0 };

        std::vector<Time> knotVector;
        for (Size i=0; i< LENGTH(knots); i++) {
            knotVector.push_back(knots[i]);
        }

        CubicBSplinesFitting cubicBSplines(knotVector, constrainAtZero);

        boost::shared_ptr<FittedBondDiscountCurve> ts4 (
                       new FittedBondDiscountCurve(curveSettlementDays,
                                                   calendar,
                                                   instrumentsA,
                                                   dc,
                                                   cubicBSplines,
                                                   tolerance,
                                                   max));

        printOutput("(d) cubic B-splines", ts4);

        SvenssonFitting svensson;

        boost::shared_ptr<FittedBondDiscountCurve> ts5 (
                        new FittedBondDiscountCurve(curveSettlementDays,
                                                    calendar,
                                                    instrumentsA,
                                                    dc,
                                                    svensson,
                                                    tolerance,
                                                    max));

        printOutput("(e) Svensson", ts5);

        Handle<YieldTermStructure> discountCurve = getDiscountCurve();
        SpreadFittingMethod nelsonSiegelSpread(boost::make_shared<NelsonSiegelFitting>(), discountCurve);
        boost::shared_ptr<FittedBondDiscountCurve> ts6 (
                        new FittedBondDiscountCurve(curveSettlementDays,
                                                    calendar,
                                                    instrumentsA,
                                                    dc,
                                                    nelsonSiegelSpread,
                                                    tolerance,
                                                    max));

        printOutput("(f) Nelson-Siegel spreaded", ts6);


        cout << "Output par rates for each curve. In this case, "
             << endl
             << "par rates should equal coupons for these par bonds."
             << endl
             << endl;

        cout << setw(6) << "tenor" << " | "
             << setw(6) << "coupon" << " | "
             << setw(6) << "bstrap" << " | "
             << setw(6) << "(a)" << " | "
             << setw(6) << "(b)" << " | "
             << setw(6) << "(c)" << " | "
             << setw(6) << "(d)" << " | "
             << setw(6) << "(e)" << " | "
             << setw(6) << "(f)" << endl;

        for (Size i=0; i<instrumentsA.size(); i++) {

            std::vector<boost::shared_ptr<CashFlow> > cfs =
                instrumentsA[i]->bond()->cashflows();

            Size cfSize = instrumentsA[i]->bond()->cashflows().size();
            std::vector<Date> keyDates;
            keyDates.push_back(bondSettlementDate);

            for (Size j=0; j<cfSize-1; j++) {
                if (!cfs[j]->hasOccurred(bondSettlementDate, false)) {
                    Date myDate =  cfs[j]->date();
                    keyDates.push_back(myDate);
                }
            }

            Real tenor = dc.yearFraction(today, cfs[cfSize-1]->date());

            cout << setw(6) << fixed << setprecision(3) << tenor << " | "
                 << setw(6) << fixed << setprecision(3)
                 << 100.*coupons[i] << " | "
                 // piecewise bootstrap
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts0,keyDates,dc) << " | "
                 // exponential splines
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts1,keyDates,dc) << " | "
                 // simple polynomial
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts2,keyDates,dc) << " | "
                 // Nelson-Siegel
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts3,keyDates,dc) << " | "
                 // cubic bsplines
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts4,keyDates,dc) << " | "
                 // Svensson
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts5,keyDates,dc)  << " | "
                 // Nelson-Siegel Spreaded
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts6,keyDates,dc) << endl;
        }

        cout << endl << endl << endl;
        cout << "Now add 23 months to today. Par rates should be "  << endl
             << "automatically recalculated because today's date "  << endl
             << "changes.  Par rates will NOT equal coupons (YTM "  << endl
             << "will, with the correct compounding), but the "     << endl
             << "piecewise yield curve par rates can be used as "   << endl
             << "a benchmark for correct par rates."
             << endl
             << endl;

        today = calendar.advance(origToday,23,Months,convention);
        Settings::instance().evaluationDate() = today;
        bondSettlementDate = calendar.advance(today, bondSettlementDays*Days);

        printOutput("(a) exponential splines", ts1);

        printOutput("(b) simple polynomial", ts2);

        printOutput("(c) Nelson-Siegel", ts3);

        printOutput("(d) cubic B-splines", ts4);

        printOutput("(e) Svensson", ts5);

        printOutput("(f) Nelson-Siegel spreaded", ts6);

        cout << endl
             << endl;


        cout << setw(6) << "tenor" << " | "
             << setw(6) << "coupon" << " | "
             << setw(6) << "bstrap" << " | "
             << setw(6) << "(a)" << " | "
             << setw(6) << "(b)" << " | "
             << setw(6) << "(c)" << " | "
             << setw(6) << "(d)" << " | "
             << setw(6) << "(e)" << " | "
             << setw(6) << "(f)" << endl;

        for (Size i=0; i<instrumentsA.size(); i++) {

            std::vector<boost::shared_ptr<CashFlow> > cfs =
                instrumentsA[i]->bond()->cashflows();

            Size cfSize = instrumentsA[i]->bond()->cashflows().size();
            std::vector<Date> keyDates;
            keyDates.push_back(bondSettlementDate);

            for (Size j=0; j<cfSize-1; j++) {
                if (!cfs[j]->hasOccurred(bondSettlementDate, false)) {
                    Date myDate =  cfs[j]->date();
                    keyDates.push_back(myDate);
                }
            }

            Real tenor = dc.yearFraction(today, cfs[cfSize-1]->date());

            cout << setw(6) << fixed << setprecision(3) << tenor << " | "
                 << setw(6) << fixed << setprecision(3)
                 << 100.*coupons[i] << " | "
                 // piecewise bootstrap
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts0,keyDates,dc) << " | "
                 // exponential splines
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts1,keyDates,dc) << " | "
                 // simple polynomial
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts2,keyDates,dc) << " | "
                 // Nelson-Siegel
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts3,keyDates,dc) << " | "
                 // cubic bsplines
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts4,keyDates,dc) << " | "
                 // Svensson
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts5,keyDates,dc) << " | "
                 // Nelson-Siegel Spreaded
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts6,keyDates,dc) << endl;
        }

        cout << endl << endl << endl;
        cout << "Now add one more month, for a total of two years " << endl
             << "from the original date. The first instrument is "  << endl
             << "now expired and par rates should again equal "     << endl
             << "coupon values, since clean prices did not change."
             << endl
             << endl;

        instrumentsA.erase(instrumentsA.begin(),
                           instrumentsA.begin()+1);
        instrumentsB.erase(instrumentsB.begin(),
                           instrumentsB.begin()+1);

        today = calendar.advance(origToday,24,Months,convention);
        Settings::instance().evaluationDate() = today;
        bondSettlementDate = calendar.advance(today, bondSettlementDays*Days);

        boost::shared_ptr<YieldTermStructure> ts00 (
              new PiecewiseYieldCurve<Discount,LogLinear>(curveSettlementDays,
                                                          calendar,
                                                          instrumentsB,
                                                          dc));

        boost::shared_ptr<FittedBondDiscountCurve> ts11 (
                  new FittedBondDiscountCurve(curveSettlementDays,
                                              calendar,
                                              instrumentsA,
                                              dc,
                                              exponentialSplines,
                                              tolerance,
                                              max));

        printOutput("(a) exponential splines", ts11);


        boost::shared_ptr<FittedBondDiscountCurve> ts22 (
                    new FittedBondDiscountCurve(curveSettlementDays,
                                                calendar,
                                                instrumentsA,
                                                dc,
                                                simplePolynomial,
                                                tolerance,
                                                max));

        printOutput("(b) simple polynomial", ts22);


        boost::shared_ptr<FittedBondDiscountCurve> ts33 (
                        new FittedBondDiscountCurve(curveSettlementDays,
                                                    calendar,
                                                    instrumentsA,
                                                    dc,
                                                    nelsonSiegel,
                                                    tolerance,
                                                    max));

        printOutput("(c) Nelson-Siegel", ts33);


        boost::shared_ptr<FittedBondDiscountCurve> ts44 (
                       new FittedBondDiscountCurve(curveSettlementDays,
                                                   calendar,
                                                   instrumentsA,
                                                   dc,
                                                   cubicBSplines,
                                                   tolerance,
                                                   max));

        printOutput("(d) cubic B-splines", ts44);

        boost::shared_ptr<FittedBondDiscountCurve> ts55 (
                       new FittedBondDiscountCurve(curveSettlementDays,
                                                   calendar,
                                                   instrumentsA,
                                                   dc,
                                                   svensson,
                                                   tolerance,
                                                   max));

        printOutput("(e) Svensson", ts55);

        boost::shared_ptr<FittedBondDiscountCurve> ts66 (
                        new FittedBondDiscountCurve(curveSettlementDays,
                                                    calendar,
                                                    instrumentsA,
                                                    dc,
                                                    nelsonSiegelSpread,
                                                    tolerance,
                                                    max));

        printOutput("(f) Nelson-Siegel spreaded", ts66);

        cout << setw(6) << "tenor" << " | "
             << setw(6) << "coupon" << " | "
             << setw(6) << "bstrap" << " | "
             << setw(6) << "(a)" << " | "
             << setw(6) << "(b)" << " | "
             << setw(6) << "(c)" << " | "
             << setw(6) << "(d)" << " | "
             << setw(6) << "(e)" << " | "
             << setw(6) << "(f)" << endl;

        for (Size i=0; i<instrumentsA.size(); i++) {

            std::vector<boost::shared_ptr<CashFlow> > cfs =
                instrumentsA[i]->bond()->cashflows();

            Size cfSize = instrumentsA[i]->bond()->cashflows().size();
            std::vector<Date> keyDates;
            keyDates.push_back(bondSettlementDate);

            for (Size j=0; j<cfSize-1; j++) {
                if (!cfs[j]->hasOccurred(bondSettlementDate, false)) {
                    Date myDate =  cfs[j]->date();
                    keyDates.push_back(myDate);
                }
            }

            Real tenor = dc.yearFraction(today, cfs[cfSize-1]->date());

            cout << setw(6) << fixed << setprecision(3) << tenor << " | "
                 << setw(6) << fixed << setprecision(3)
                 << 100.*coupons[i+1] << " | "
                 // piecewise bootstrap
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts00,keyDates,dc) << " | "
                 // exponential splines
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts11,keyDates,dc) << " | "
                 // simple polynomial
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts22,keyDates,dc) << " | "
                 // Nelson-Siegel
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts33,keyDates,dc) << " | "
                 // cubic bsplines
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts44,keyDates,dc) << " | "
                 // Svensson
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts55,keyDates,dc) << " | "
                 // Nelson-Siegel Spreaded
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts66,keyDates,dc) << endl;
        }


        cout << endl << endl << endl;
        cout << "Now decrease prices by a small amount, corresponding"  << endl
             << "to a theoretical five basis point parallel + shift of" << endl
             << "the yield curve. Because bond quotes change, the new " << endl
             << "par rates should be recalculated automatically."
             << endl
             << endl;

        for (Size k=0; k<LENGTH(lengths)-1; k++) {

            Real P = instrumentsA[k]->quote()->value();
            const Bond& b = *instrumentsA[k]->bond();
            Rate ytm = BondFunctions::yield(b, P,
                                            dc, Compounded, frequency,
                                            today);
            Time dur = BondFunctions::duration(b, ytm,
                                               dc, Compounded, frequency,
                                               Duration::Modified,
                                               today);

            const Real bpsChange = 5.;
            // dP = -dur * P * dY
            Real deltaP = -dur * P * (bpsChange/10000.);
            quote[k+1]->setValue(P + deltaP);
        }


        cout << setw(6) << "tenor" << " | "
             << setw(6) << "coupon" << " | "
             << setw(6) << "bstrap" << " | "
             << setw(6) << "(a)" << " | "
             << setw(6) << "(b)" << " | "
             << setw(6) << "(c)" << " | "
             << setw(6) << "(d)" << " | "
             << setw(6) << "(e)" << " | "
             << setw(6) << "(f)" << endl;

        for (Size i=0; i<instrumentsA.size(); i++) {

            std::vector<boost::shared_ptr<CashFlow> > cfs =
                instrumentsA[i]->bond()->cashflows();

            Size cfSize = instrumentsA[i]->bond()->cashflows().size();
            std::vector<Date> keyDates;
            keyDates.push_back(bondSettlementDate);

            for (Size j=0; j<cfSize-1; j++) {
                if (!cfs[j]->hasOccurred(bondSettlementDate, false)) {
                    Date myDate =  cfs[j]->date();
                    keyDates.push_back(myDate);
                }
            }

            Real tenor = dc.yearFraction(today, cfs[cfSize-1]->date());

            cout << setw(6) << fixed << setprecision(3) << tenor << " | "
                 << setw(6) << fixed << setprecision(3)
                 << 100.*coupons[i+1] << " | "
                 // piecewise bootstrap
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts00,keyDates,dc) << " | "
                 // exponential splines
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts11,keyDates,dc) << " | "
                 // simple polynomial
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts22,keyDates,dc) << " | "
                 // Nelson-Siegel
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts33,keyDates,dc) << " | "
                 // cubic bsplines
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts44,keyDates,dc) << " | "
                 // Svensson
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts55,keyDates,dc) << " | "
                 // Nelson-Siegel Spreaded
                 << setw(6) << fixed << setprecision(3)
                 << 100.*parRate(*ts66,keyDates,dc) << endl;
        }


        double seconds = timer.elapsed();
        Integer hours = int(seconds/3600);
        seconds -= hours * 3600;
        Integer minutes = int(seconds/60);
        seconds -= minutes * 60;
        std::cout << " \nRun completed in ";
        if (hours > 0)
            std::cout << hours << " h ";
        if (hours > 0 || minutes > 0)
            std::cout << minutes << " m ";
        std::cout << std::fixed << std::setprecision(0)
                  << seconds << " s\n" << std::endl;

        return 0;

    } catch (std::exception& e) {
        cerr << e.what() << endl;
        return 1;
    } catch (...) {
        cerr << "unknown error" << endl;
        return 1;
    }

}

