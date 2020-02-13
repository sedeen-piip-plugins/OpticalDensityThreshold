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

#include "ODThresholdKernel.h"
#include "ODConversion.h"

//C++ headers
#include <cassert>
#include <numeric>

// User header
#include "geometry/graphic/Rectangle.h"
#include "global/ColorSpace.h"
#include "global/Debug.h"
#include "global/RawImage.h"
#include "global/geometry/Point.h"
#include "global/geometry/Rect.h"
#include "global/geometry/SRTTransform.h"
#include "global/geometry/Size.h"
#include "image/iterator/Iterator.h"
#include "image/tile/Factory.h"

namespace sedeen {
namespace image {

namespace {
    const ColorSpace OutputColor(ColorModel::RGBA, ChannelType::UInt8); //Output ColorSpace
}

namespace tile {
    
ODThresholdKernel::ODThresholdKernel(double ODThreshVal, 
    Behavior behavior, std::array<double, 3> weights /*= { 1.0,1.0,1.0 }*/) :
    m_odThreshVal(ODThreshVal),
    m_behavior(behavior),
    m_weightVals(weights) {
}//end constructor

ODThresholdKernel::~ODThresholdKernel(void) {
}//end destructor

void ODThresholdKernel::setODThreshold(double v) {
    if (m_odThreshVal != v) {
        m_odThreshVal = v;
        update();
    }
}//end setODThreshold

void ODThresholdKernel::setBehavior(Behavior t) {
    if (t != m_behavior) {
        m_behavior = t;
        update();
    }
}//end setBehavior

void ODThresholdKernel::setWeights(std::array<double, 3> w) {
    if (w != m_weightVals) {
        m_weightVals = w;
        update();
    }
}//end setWeights

RawImage ODThresholdKernel::doProcessData(const RawImage &source)
{
    //Get the ColorSpace and ColorModel of the source
    ColorSpace sourceColorSpace = source.colorSpace();
    ColorModel sourceColorModel = sourceColorSpace.colorModel();

    //Get the pixel order of the source image: Interleaved or Planar
    PixelOrder pixelOrder = source.order();
    auto numElements = source.count();
    int numSourceChannels = static_cast<int>(channels(source));
    int numPixels = static_cast<int>(numElements / numSourceChannels);
    sedeen::Size imageSize = source.size();

    // Construct the output buffer (copy source properties)
    auto buffer = std::make_shared<RawImage>(imageSize, doGetColorSpace(), pixelOrder);
    buffer->fill(ChannelValue(0));

    //Properties of the output image
    int numOutputChannels = static_cast<int>(channels(*buffer));
    int outputScaleMax = sedeen::maxChannelValue<int>(doGetColorSpace());

    //Perform faster OD conversions using a lookup table
    std::shared_ptr<ODConversion> converter = std::make_shared<ODConversion>();

    //Check the sum of the weights. Is it zero? Set denominator to 1.0 instead if so
    double weightSum = std::accumulate(m_weightVals.begin(), m_weightVals.end(), 0.0);
    double weightDenominator = (weightSum == 0.0) ? 1.0 : weightSum;

    //Loop through all pixels in the source
    for (int px = 0; px < numPixels; px++) {
        //Sets of indices corresponding to the elements of a pixel in the source and output
        std::vector<int> sourceIndices, outputIndices, srcToOutIndex;
        //If source is Grayscale, use source channel 0 for all.
        if ((sourceColorModel == ColorModel::Grayscale) || (numSourceChannels == 1)) {
            srcToOutIndex = { 0,0,0 };
        }
        else {
            srcToOutIndex = { 0,1,2 };
        }
        int srcToOutSize = static_cast<int>(srcToOutIndex.size());

        //Loop through channels, get indices based on PixelOrder
        if (pixelOrder == PixelOrder::Interleaved) {
            for (int ch = 0; ch < srcToOutSize; ch++) {
                //RGB RGB RGB ... (if numChannels=3)
                //index.push_back(pixel*numChannels + channel);
                sourceIndices.push_back(px*numSourceChannels + srcToOutIndex.at(ch));
                outputIndices.push_back(px*numOutputChannels + ch);
            }
            outputIndices.push_back(px*numOutputChannels + srcToOutSize);
        }
        else if (pixelOrder == PixelOrder::Planar) {
            for (int ch = 0; ch < srcToOutSize; ch++) {
                //RRR... GGG... BBB...
                //index.push_back(channel*numPixels + pixel);
                sourceIndices.push_back(srcToOutIndex.at(ch)*numPixels + px);
                outputIndices.push_back(ch*numPixels + px);
            }
            outputIndices.push_back(srcToOutSize*numPixels + px);
        }
        else {
            //Invalid value of pixelOrder
            assert(false && "Invalid PixelOrder defined");
        }

        //Compute the value to compare to the threshold
        double w_odRunningTotal(0.0), w_odVal(0.0);
        //Loop over the number of channels that should contribute to the comparison value
        for (int ch = 0; ch < sourceIndices.size(); ch++) {
            //Get the channel in the source image that should be accessed
            w_odRunningTotal += m_weightVals.at(ch) 
                * converter->LookupRGBtoOD(source.at(sourceIndices.at(ch)).as<int>());
        }
        w_odVal = w_odRunningTotal / weightDenominator;

        //Check w_odVal against the m_odThreshVal, assign 
        if ( ((m_behavior == RETAIN_LOWER_OD)  && (w_odVal <= m_odThreshVal))
          || ((m_behavior == RETAIN_HIGHER_OD) && (w_odVal >= m_odThreshVal)) ) {
            for (int si = 0; si < sourceIndices.size(); si++) {
                buffer->setValue(outputIndices.at(si), source.at(sourceIndices.at(si)));
            }
        }
        //Set the last element of each output pixel
        buffer->setValue(outputIndices.back(), outputScaleMax);
    }//end for px

    return *buffer;
}//end doProcessData

const ColorSpace& ODThresholdKernel::doGetColorSpace() const {
    return OutputColor;
}

} // namespace tile

} // namespace image
} // namespace sedeen

