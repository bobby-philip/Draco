//----------------------------------*-C++-*----------------------------------//
/*!
 * \file   cdi_ipcress/test/ReadOdfIpcressFile.cc
 * \author Seth R. Johnson
 * \date   Thu July 10 2008
 * \brief  
 */
//---------------------------------------------------------------------------//
// $Id: ReadOdfIpcressFile.cc
//---------------------------------------------------------------------------//

#include "cdi_ipcress_test.hh"
#include "../IpcressFile.hh"
#include "../IpcressOdfmgOpacity.hh"
#include "cdi/OpacityCommon.hh"
#include "ds++/Release.hh"
#include "ds++/Assert.hh"
#include "ds++/SP.hh"
#include "ds++/ScalarUnitTest.hh"

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdio>
#include <string>

#define PASSMSG(m) ut.passes(m)
#define FAILMSG(m) ut.failure(m)

using rtt_cdi_ipcress::IpcressOdfmgOpacity;
using rtt_cdi_ipcress::IpcressFile;
using rtt_dsxx::SP;

using std::cerr;
using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::istringstream;
using std::ostringstream;

typedef SP<const IpcressOdfmgOpacity>           SP_Goo;
typedef std::vector< double >                   vec_d;
typedef std::vector< std::vector<double> >      vec2_d;

void printGrid(SP_Goo spGandOpacity);
void askTempDens(double &temperature, double &density);
void analyzeData(SP_Goo spGandOpacity);
void collapseOpacities(SP_Goo spGandOpacity, double temperature,
                       double density);
void printCData(SP_Goo spGandOpacity, double temperature, double density);
void printData (SP_Goo spGandOpacity, double temperature, double density);
void printTable(SP_Goo spGandOpacity, double temperature, double density);

//---------------------------------------------------------------------------//
void printSyntax()
{
    cerr << "syntax: ReadOdfIpcressFile [--help] [--bands n | --mg] "
         << "[--analyze | --printc | --printtable | --collapse] [--model "
         << "(r[osseland] | p[lanck])] [--reaction (t[otal] | a[bsorption]"
         << " | s[cattering])] [-d density -t temperature] fileName" << endl;
}
//---------------------------------------------------------------------------//

int main(int argc, char *argv[])
{
    rtt_dsxx::ScalarUnitTest ut(argc, argv, rtt_dsxx::release);
    if (argc <= 1)
    {
        cerr << "Must have at least one input argument (the ipcress file)."
             << endl;
        printSyntax();
        PASSMSG("In case this is an automated test...");
        return ut.numFails;
    }
    else if (argc == 2)
    {
        // test to see if it's just "--help"
        string currentArg = argv[1];
        if (currentArg == "--help")
        {
            printSyntax();
            return 0;
        }
    }

    // get the ipcress file name, and create the ipcress file
    string ipcressFileName = argv[argc - 1];
    SP<const IpcressFile> file;
    try
    {
        file = new IpcressFile(ipcressFileName);
    }
    catch ( rtt_dsxx::assertion const & excpt )
    {
        ostringstream message;
        message << "Failed to create SP to new IpcressFile object for "
                << "file \"" << ipcressFileName << "\":"
                << excpt.what();
        FAILMSG(message.str());
        cerr << "Aborting program.";
        return 1;
    }

    // data needed to load the Ipcress file properly
    int numBands = 0;
    rtt_cdi::Model model = rtt_cdi::ROSSELAND;
    rtt_cdi::Reaction reaction = rtt_cdi::ABSORPTION;
    int matID = 19000;
    int actionToTake = 0; // 0 for user input, 1 for analyze, 2 for print c

    double temperature = 0;
    double density = 0;

    // loop on all arguments except the first (program name) and last (input
    // file name) 
    for (int arg = 1; arg < argc - 1; arg++)
    {
        string currentArg = argv[arg];

        if (currentArg == "--help")
        {
            printSyntax();
            return 0;
        }
        else if (currentArg == "-d")
        {
            arg++; // start looking at next argument

            istringstream inString(argv[arg]);
            inString >> density;
            cerr << "Using density of " << density << endl;
        }
        else if (currentArg == "-t")
        {
            arg++; // start looking at next argument

            istringstream inString(argv[arg]);
            inString >> temperature;
            cerr << "Using temperature of " << temperature << endl;
        }
        else if (currentArg == "--model")
        {
            arg++; // start looking at next argument

            if (argv[arg][0] == 'r')
            {
                model = rtt_cdi::ROSSELAND;
            }
            else if (argv[arg][0] == 'p')
            {
                model = rtt_cdi::PLANCK;
            }
            else
            {
                cerr << "Unrecognized model option '"
                     << argv[arg] << "'" << endl;
                cerr << "Defaulting to rosseland" << endl;
            }
        }
        else if (currentArg == "--reaction")
        {
            arg++; // start looking at next argument

            if (argv[arg][0] == 'a')
            {
                reaction = rtt_cdi::ABSORPTION;
            }
            else if (argv[arg][0] == 's')
            {
                reaction = rtt_cdi::SCATTERING;
            }
            else if (argv[arg][0] == 't')
            {
                reaction = rtt_cdi::TOTAL;
            }
            else
            {
                cerr << "Unrecognized model option '"
                     << argv[arg] << "'" << endl;
                cerr << "Defaulting to rosseland" << endl;
            }

        }
        else if (currentArg == "--mg")
        {
            numBands = 1;
            cerr << "Using " << numBands << " bands (multigroup file)" << endl;
        }
        else if (currentArg == "--bands")
        {
            arg++; // start looking at next argument

            istringstream inString(argv[arg]);
            inString >> numBands;
            cerr << "Using " << numBands << " bands" << endl;
        }
        else if  (currentArg == "--analyze")
        {
            actionToTake = 1;
        }
        else if  (currentArg == "--printc")
        {
            actionToTake = 2;
        }
        else if  (currentArg == "--collapse")
        {
            actionToTake = 3;
        }
        else if  (currentArg == "--printtable")
        {
            actionToTake = 4;
        }
        else
        {
            cerr << "Unrecognized option \"" << currentArg << "\"." << endl;
        }
    }

    //print the model that we're using
    if (model == rtt_cdi::ROSSELAND)
    {
        cerr << "Using ROSSELAND weighting" << endl;
    }
    else
    {
        cerr << "Using PLANCK weighting" << endl;
    }
        
    //print the cross section that we're using
    if (reaction == rtt_cdi::TOTAL)
    {
        cerr << "Using TOTAL reaction" << endl;
    }
    else if (reaction == rtt_cdi::ABSORPTION)
    {
        cerr << "Using ABSORPTION reaction" << endl;
    }
    else
    {
        cerr << "Using SCATTERING reaction" << endl;
    }

    //ask the user for the number of bands
    while (numBands == 0)
    {
        cout << "Enter the number of bands (use 1 for multigroup file): ";
        cin >> numBands;
    }
    Insist(numBands > 0, "Must have a positive number of bands.");

    //load the Ipcress ODFMG Opacity
    SP<const IpcressOdfmgOpacity> spGandOpacity;

    spGandOpacity = new IpcressOdfmgOpacity(file,
                                            matID,
                                            model,
                                            reaction,
                                            numBands);

    cerr        << "Successfully read Ipcress file \"" << ipcressFileName 
                << "\"." << endl;

    switch (actionToTake)
    {
        case 0:
            if (temperature == 0 || density == 0)
            {
                printGrid(spGandOpacity);
                askTempDens(temperature, density);
            }
            printData(spGandOpacity, temperature, density);
            break;
        case 1:
            analyzeData(spGandOpacity);
            break;
        case 2:
            if (temperature == 0 || density == 0)
            {
                printGrid(spGandOpacity);
                askTempDens(temperature, density);
            }
            printCData(spGandOpacity, temperature, density);
            break;
        case 3:
            if (temperature == 0 || density == 0)
            {
                printGrid(spGandOpacity);
                askTempDens(temperature, density);
            }
            collapseOpacities(spGandOpacity, temperature, density);
            break;
        case 4:
            if (temperature == 0 || density == 0)
            {
                printGrid(spGandOpacity);
                askTempDens(temperature, density);
            }
            printTable(spGandOpacity, temperature, density);

            break;
    }

    cerr        << "Finished." << endl;

    return 0;
}
//---------------------------------------------------------------------------//
void printGrid(SP_Goo spGandOpacity)
{
    const vec_d temperatures = spGandOpacity->getTemperatureGrid();
    const vec_d densities    = spGandOpacity->getDensityGrid();

    cout << "Temperature grid: ";
    for (size_t i = 0; i < temperatures.size(); i++)
        cout << temperatures[i] << " ";
    cout << endl;

    cout << "Density grid: ";
    for (size_t i = 0; i < densities.size(); i++)
        cout << densities[i] << " ";
    cout << endl;
}
//---------------------------------------------------------------------------//
void askTempDens(double &temperature, double &density)
{
    while (temperature <=0 || density <= 0)
    {
        cout << "Enter the temperature to analyze: ";
        cin >> temperature;

        cout << "Enter the density to analyze    : ";
        cin >> density;
    }
}
//---------------------------------------------------------------------------//
void analyzeData(SP_Goo spGandOpacity)
{
    Require(spGandOpacity);

    const int numBands  = spGandOpacity->getNumBands();
    const int numGroups = spGandOpacity->getNumGroups();

    const vec_d temperatures = spGandOpacity->getTemperatureGrid();
    const vec_d densities    = spGandOpacity->getDensityGrid();

    cout << "Temperature\tDensity\tGroup opacity range\n";

    //loop over all stored temperatures and densities
    for (size_t t = 0; t < temperatures.size(); t++)
    {
        for (size_t d = 0; d < densities.size(); d++)
        {
            vec2_d multiBandOpacities = 
                spGandOpacity->getOpacity(temperatures[t], densities[d]);
            cout << temperatures[t] << "\t" << densities[d] << "\t";

            for (int group = 0; group < numGroups; group++) 
            {
                cout    << multiBandOpacities[group][numBands - 1] /
                    multiBandOpacities[group][0]
                        << "\t";
            }
            cout << endl;
        }
    }
}
//---------------------------------------------------------------------------//
void collapseOpacities(SP_Goo spGandOpacity,
                       double temperature,
                       double density)
{
    Require(spGandOpacity);

    const int numBands   = spGandOpacity->getNumBands();
    const int numGroups  = spGandOpacity->getNumGroups();
    const rtt_cdi::Model model = spGandOpacity->getModelType();

    cout << "=============================================" << endl;
    cout << "Printing collapsed opacities at " << temperature << " keV, "
         << "rho = " << density << endl;
    cout << "=============================================" << endl;

    const vec_d groupBoundaries  = spGandOpacity->getGroupBoundaries();
    const vec_d bandBoundaries  = spGandOpacity->getBandBoundaries();
        
    vec_d bandWidths(numBands, 0.0);

    //calculate band widths
    Check(static_cast<size_t>(numBands + 1) == bandBoundaries.size());

    for (int band = 0; band < numBands ; band++)
    {
        bandWidths[band] = bandBoundaries[band+1] - bandBoundaries[band];
    }
        
    //loop over groups
    vec2_d multiBandOpacities = 
        spGandOpacity->getOpacity(temperature, density);

    for (int group = 0; group < numGroups; group++) 
    {
        double collapsedOpacity = 0;

        // harmonic average for rosseland
        if (model == rtt_cdi::ROSSELAND)
        {
            for (int band = numBands - 1; band >= 0; band--)
            {
                collapsedOpacity += bandWidths[band] /
                                    multiBandOpacities[group][band];
            }
            if (collapsedOpacity != 0.0)
                collapsedOpacity = 1/collapsedOpacity;
        }
        else // arithmetic average for planckian
        {
            for (int band = 0; band < numBands ; band++)
            {
                collapsedOpacity += bandWidths[band] *
                                    multiBandOpacities[group][band];
            }
        }

        printf("%4d\t%.6g", group + 1, collapsedOpacity);
        cout << endl;
    }
}
//---------------------------------------------------------------------------//
void printTable(SP_Goo spGandOpacity, double temperature, double density)
{
    Require(spGandOpacity);

    const int numBands  = spGandOpacity->getNumBands();
    const int numGroups = spGandOpacity->getNumGroups();

    cout << "Temperature:\t" << temperature << "\tDensity:\t"
         << density << endl;

    // print group boundaries
    cout << "numGroups:\t" << numGroups << "\tnumBands:\t" << numBands << endl;

    const vec_d groupBoundaries  = spGandOpacity->getGroupBoundaries();

    for (size_t i = 0; i < groupBoundaries.size(); i++)
    {
        printf("%.6g\t", groupBoundaries[i]);
    }
    cout  << endl;
        
    const vec_d bandBoundaries  = spGandOpacity->getBandBoundaries();
        
    for (size_t i = 0; i < bandBoundaries.size(); i++)
    {
        printf("%.6g\t", bandBoundaries[i]);
    }
    cout  << endl;

    // print opacity data
    vec2_d multiBandOpacities = 
        spGandOpacity->getOpacity(temperature, density);

    for (int group = 0; group < numGroups; group++) 
    {
        // print data for each band
        for (int band = 0; band < numBands ; band++)
        {
            printf("%.16g\t", multiBandOpacities[group][band]);
        }

        cout << endl;
    }
    cout << endl;
}

//---------------------------------------------------------------------------//
void printCData(SP_Goo spGandOpacity, double temperature, double density)
{
    Require(spGandOpacity);

    const int numBands  = spGandOpacity->getNumBands();
    const int numGroups = spGandOpacity->getNumGroups();

    cout        << "=============================================" << endl;
    cout        << "Printing data at " << temperature << " keV, "
                << "rho = " << density << endl;
    cout        << "=============================================" << endl;

    // print group boundaries
    cout  << "const int numGroups = " << numGroups << ";" << endl;
    cout  << "const double groupBoundaries[numGroups + 1] = {" << endl;

    const vec_d groupBoundaries  = spGandOpacity->getGroupBoundaries();

    for (size_t i = 0; i < groupBoundaries.size(); i++)
    {
        printf("\t%.6g", groupBoundaries[i]);
        if (i != groupBoundaries.size() - 1)
            cout << ",";

        cout << endl;
    }
    cout  << "};" << endl;
        
    // print band boundaries

    cout  << "const int numBands = " << numBands << ";" << endl;
    cout  << "const double bandBoundaries[numBands + 1] = {" << endl;

    const vec_d bandBoundaries  = spGandOpacity->getBandBoundaries();
        
    for (size_t i = 0; i < bandBoundaries.size(); i++)
    {
        printf("\t%.6g", bandBoundaries[i]);
        if (i != bandBoundaries.size() - 1)
            cout << ",";

        cout << endl;
    }
    cout  << "};" << endl;

    // print opacity data
    cout  << "const double opacities[numGroups][numBands] = {" << endl;

    vec2_d multiBandOpacities = 
        spGandOpacity->getOpacity(temperature, density);

    for (int group = 0; group < numGroups; group++) 
    {
        cout << "{" << endl;
        // print data for each band
        for (int band = 0; band < numBands ; band++)
        {
            printf("\t%#25.16g", multiBandOpacities[group][band]);

            if (band != numBands - 1)
                cout << ",";

            printf("\t\t// group %d band %d", group + 1, band + 1);

            cout << endl;
        }

        cout << "}";
        if (group != numGroups - 1)
            cout << ",";

        cout << endl;
    }
    cout << "};" << endl;
}

//---------------------------------------------------------------------------//
void printData(SP_Goo spGandOpacity, double temperature, double density)
{
    Require(spGandOpacity);

    const int numBands  = spGandOpacity->getNumBands();
    const int numGroups = spGandOpacity->getNumGroups();

    const vec_d groupBoundaries = spGandOpacity->getGroupBoundaries();
    const vec_d bandBoundaries  = spGandOpacity->getBandBoundaries();

    cout        << "=============================================" << endl;
    cout        << "Printing band data at " << temperature << " keV, "
                << "rho = " << density << endl;

    vec2_d multiBandOpacities = 
        spGandOpacity->getOpacity(temperature, density);

    int    maxGroup = 0;
    double maxRatio = 0.0;

    for (int group = 0; group < numGroups; group++) 
    {
        double currentRatio = 0.0;

        cout << "=== Group " << group + 1 << " has energy range ["
             << groupBoundaries[group] << ","
             << groupBoundaries[group + 1] <<"]\n";
                
        cout << "Group Band  Width        Opacity  Ratio to first"
             << std::endl;

        // print data for each band
        for (int band = 0; band < numBands ; band++)
        {
            currentRatio = multiBandOpacities[group][band] /
                           multiBandOpacities[group][0];

            printf("%5d %4d %6.3f %#14.6G  %14.3f\n",
                   group + 1, band + 1,
                   bandBoundaries[band + 1] - bandBoundaries[band],
                   multiBandOpacities[group][band],
                   currentRatio);
        }

        // compare result for last band
        if (currentRatio > maxRatio)
        {
            maxRatio = currentRatio;
            maxGroup = group;
        }
    }
        
    cout << "=============================================" << endl;
    cout << "At " << temperature << " keV, "
         << "rho = " << density << endl;
    cout << "Best odf was in group " << maxGroup << " which had a high-to-low "
         << "ratio of " << maxRatio << "." << endl;
    cout << "=============================================" << endl;
}

//---------------------------------------------------------------------------//
//                        end of ReadOdfIpcressFile.cc
//---------------------------------------------------------------------------//