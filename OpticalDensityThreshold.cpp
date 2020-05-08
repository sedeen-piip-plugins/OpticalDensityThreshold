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

#include "OpticalDensityThreshold.h"

//Sedeen required headers
#include "Algorithm.h"
#include "Geometry.h"
#include "Global.h"
#include "Image.h"
#include "image/io/Image.h"
#include "image/tile/Factory.h"

// Poco header needed for the macros below 
#include <Poco/ClassLibrary.h>

// Declare that this object has AlgorithmBase subclasses
//  and declare each of those sub-classes
POCO_BEGIN_MANIFEST(sedeen::algorithm::AlgorithmBase)
POCO_EXPORT_CLASS(sedeen::algorithm::OpticalDensityThreshold)
POCO_END_MANIFEST

namespace sedeen {
namespace algorithm {

//Constructor
OpticalDensityThreshold::OpticalDensityThreshold() 
    : m_displayArea(),
    m_regionToProcess(),
    m_threshold(),
    m_retainment(),
    m_thresholdType(),
    m_RWeight(),
    m_GWeight(),
    m_BWeight(),
    m_result(),
    m_outputText(),
    m_report(""),
    m_thresholdDefaultVal(20.0),
    m_thresholdMaxVal(300.0),
    m_ODThreshold_factory(nullptr)
{
    //List the options for retainment type
    m_retainmentOptions.push_back("Lower OD (retain lighter)");
    m_retainmentOptions.push_back("Higher OD (retain darker)");

    m_thresholdTypeOptions.push_back("Average OD");
    m_thresholdTypeOptions.push_back("Weighted Average OD");
}//end  constructor

 //Destructor
OpticalDensityThreshold::~OpticalDensityThreshold() {
}

void OpticalDensityThreshold::init(const image::ImageHandle& image) {
    if (isNull(image)) return;
    // bind algorithm members to UI and initialize their properties

    // Bind system parameter for current view
    m_displayArea = createDisplayAreaParameter(*this);

    //Assemble the user interface
    m_thresholdType = createOptionParameter(*this, "Threshold type",
        "Choose how to calculate the integrated optical density: average, or use uneven weights for the RGB pixel components",
        0, m_thresholdTypeOptions, false);

    m_retainment = createOptionParameter(*this, "Retain pixels",
        "Choose whether to retain pixels with lower (lighter) or higher (darker) optical density", 
        1, m_retainmentOptions, false);

    m_threshold = createDoubleParameter(*this,
        "OD x100 Threshold",   // Widget label
        "A Threshold value",   // Widget tooltip
        20,   // Initial value
        0.0,  // minimum value
        300,  // maximum value
        false);

    m_RWeight = createDoubleParameter(*this,
        "Red weight",   // Widget label
        "Weight to apply to the Red optical density component of the pixel when comparing to the threshold value",
        1.0,   // Initial value
        0.0,   // minimum value
        10.0,  // maximum value
        false);

    m_GWeight = createDoubleParameter(*this,
        "Green weight",   // Widget label
        "Weight to apply to the Green optical density component of the pixel when comparing to the threshold value",
        1.0,   // Initial value
        0.0,   // minimum value
        10.0,  // maximum value
        false);

    m_BWeight = createDoubleParameter(*this,
        "Blue weight",   // Widget label
        "Weight to apply to the Blue optical density component of the pixel when comparing to the threshold value",
        1.0,   // Initial value
        0.0,   // minimum value
        10.0,  // maximum value
        false);

    //GraphicItemParameter m_regionToProcess; //single output region
    m_regionToProcess = createGraphicItemParameter(*this, "Apply to ROI (None for Display Area)",
        "Choose a Region of Interest on which to apply the stain separation algorithm. Choosing no ROI will apply the stain separation to the whole slide image.",
        true); //optional. None means apply to whole slide

    // Bind result
    m_outputText = createTextResult(*this, "Text Result");
    m_result = createImageResult(*this, " StainAnalysisResult");

}//end init

void OpticalDensityThreshold::run() {
    // Has display area changed
    bool display_changed = m_displayArea.isChanged();

    //Have any parameters been changed
    bool pipeline_changed = buildPipeline();

    if (display_changed || pipeline_changed) {
        m_result.update(m_ODThreshold_factory, m_displayArea, *this);
        // Update the output text report
        if (false == askedToStop()) {
            //auto report = generateCompleteReport();
            //m_outputText.sendText(report);

            // Get image from the output factory
            auto compositor = std::make_unique<image::tile::Compositor>(m_ODThreshold_factory);

            DisplayRegion region = m_displayArea;
            auto output_image = compositor->getImage(region.source_region, region.output_size);

            // Get image from the input factory
            //auto compositorsource = std::make_unique<image::tile::Compositor>(image()->getFactory());
            //auto input_image = compositorsource->getImage(region.source_region, region.output_size);

            if (m_regionToProcess.isUserDefined()) {
                 std::shared_ptr<GraphicItemBase> roi = m_regionToProcess;
                auto display_resolution = getDisplayResolution(image(), m_displayArea);
                Rect rect = containingRect(roi->graphic());
                output_image = compositor->getImage(rect, region.output_size);
            }
        }
    }//if display or pipeline changed

    // Ensure we run again after an abort
    if (askedToStop()) {
        m_ODThreshold_factory.reset();
    }
}//end run

bool OpticalDensityThreshold::buildPipeline() {
    using namespace image::tile;
    bool pipeline_changed = false;

    // Get source image properties
    auto source_factory = image()->getFactory();
    auto source_color = source_factory->getColorSpace();

    bool doProcessing = false;
    if (pipeline_changed
        || m_regionToProcess.isChanged()
        || m_threshold.isChanged()
        || m_displayArea.isChanged()
        || m_retainment.isChanged()
        || m_thresholdType.isChanged()
        || m_RWeight.isChanged()
        || m_GWeight.isChanged()
        || m_BWeight.isChanged()
        || (nullptr == m_ODThreshold_factory)) 
    {
        auto display_resolution = getDisplayResolution(image(), m_displayArea);

        //Get the Behavior value from the m_retainment
        int retainmentOptionNum = m_retainment;
        ODThresholdKernel::Behavior behaviorVal;
        if (retainmentOptionNum == ODThresholdKernel::Behavior::RETAIN_LOWER_OD) {
            behaviorVal = ODThresholdKernel::Behavior::RETAIN_LOWER_OD;
        }
        else if (retainmentOptionNum == ODThresholdKernel::Behavior::RETAIN_HIGHER_OD) {
            behaviorVal = ODThresholdKernel::Behavior::RETAIN_HIGHER_OD;
        }
        else {
            //Unknown state. Do nothing.
            behaviorVal = ODThresholdKernel::Behavior::NO_ACTION;
        }

        std::array<double, 3> theWeights = { m_RWeight, m_GWeight, m_BWeight };
        //Scale down the threshold to create more precision
        auto threshold_kernel =
            std::make_shared<image::tile::ODThresholdKernel>(m_threshold / 100.0,
            behaviorVal, theWeights);

        // Create a Factory for the composition of these Kernels
        auto non_cached_factory =
            std::make_shared<FilterFactory>(source_factory, threshold_kernel);

        // Wrap resulting Factory in a Cache for speedy results
        m_ODThreshold_factory =
            std::make_shared<Cache>(non_cached_factory, RecentCachePolicy(30));

        pipeline_changed = true;
    }//end if parameter values changed

    //
    // Constrain processing to the region of interest provided, if set
    std::shared_ptr<GraphicItemBase> region = m_regionToProcess;
    if (pipeline_changed && (nullptr != region)) {
        // Constrain the output of the pipeline to the region of interest provided
        auto constrained_factory = std::make_shared<RegionFactory>(m_ODThreshold_factory, region->graphic());

        // Wrap resulting Factory in a Cache for speedy results
        m_ODThreshold_factory = std::make_shared<Cache>(constrained_factory, RecentCachePolicy(30));
    }

    return pipeline_changed;
}//end buildPipeline

} // namespace algorithm
} // namespace sedeen
