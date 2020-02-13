/*=============================================================================
 *
 *  Copyright (c) 2020 Sunnybrook Research Institute
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 *=============================================================================*/

#ifndef SEDEEN_SRC_PLUGINS_OPTICALDENSITYTHRESHOLD_OPTICALDENSITYTHRESHOLD_H
#define SEDEEN_SRC_PLUGINS_OPTICALDENSITYTHRESHOLD_OPTICALDENSITYTHRESHOLD_H

//Sedeen required headers
#include "algorithm/AlgorithmBase.h"
#include "algorithm/Parameters.h"
#include "algorithm/Results.h"

#include "ODThresholdKernel.h"

namespace sedeen {
namespace tile {

} // namespace tile

namespace algorithm {

///OpticalDensityThreshold plugin for Sedeen Viewer
class OpticalDensityThreshold : public algorithm::AlgorithmBase {
public:
	// Constructor
    OpticalDensityThreshold();
	// Destructor
	virtual ~OpticalDensityThreshold();

	// Other public methods you would like to include

    

private:
	//Private methods
	virtual void init(const image::ImageHandle& image);
    virtual void run();
    ///Updates which GUI parameters should be displayed/hidden
    //virtual void update();

    /// Creates the thresholding pipeline with a cache
    //
    /// \return 
    /// TRUE if the pipeline has changed since the call to this function, FALSE
    /// otherwise
    bool buildPipeline();

private:
    DisplayAreaParameter m_displayArea;

    GraphicItemParameter m_regionToProcess; //single output region

    /// User defined Threshold value.
    algorithm::DoubleParameter m_threshold;
    ///Option to specify if higher or lower OD values should be retained
    algorithm::OptionParameter m_retainment;
    /// Specify the type of threshold. How should multiple pixel elements be treated?
    algorithm::OptionParameter m_thresholdType;

    ///Three weighting values
    algorithm::DoubleParameter m_RWeight;
    algorithm::DoubleParameter m_GWeight;
    algorithm::DoubleParameter m_BWeight;

    /// The output result
    ImageResult m_result;
    TextResult m_outputText;
    std::string m_report;

    /// The intermediate image factory after thresholding
    std::shared_ptr<image::tile::Factory> m_ODThreshold_factory;

private:
    //Member variables
    std::vector<std::string> m_retainmentOptions;
    std::vector<std::string> m_thresholdTypeOptions;
    double m_thresholdDefaultVal;
    double m_thresholdMaxVal;

};

} // namespace algorithm
} // namespace sedeen

#endif
