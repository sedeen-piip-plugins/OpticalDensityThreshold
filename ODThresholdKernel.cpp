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
    //What channel of each source pixel should be used for the output pixels?
    //If source is Grayscale, use channel 0 for all. If source is GBR, reverse order
    std::vector<int> translateSourceToOutputChannels;
    if ((sourceColorModel == ColorModel::Grayscale) || (numSourceChannels == 1)) {
        translateSourceToOutputChannels = { 0,0,0 };
    }
    else if (sourceColorModel == ColorModel::BGR) {
        translateSourceToOutputChannels = { 2,1,0 };
    }
    else {
        translateSourceToOutputChannels = { 0,1,2 };
    }

    //Perform faster OD conversions using a lookup table
    std::shared_ptr<ODConversion> converter = std::make_shared<ODConversion>();

    //Check the sum of the weights. Is it zero? Set denominator to 1.0 instead if so
    double weightSum = std::accumulate(m_weightVals.begin(), m_weightVals.end(), 0.0);
    double weightDenominator = (weightSum == 0.0) ? 1.0 : weightSum;



    //Loop over positions in the source image
    int y = 0, x = 0;
    for (int j = 0; j < imageSize.width()*imageSize.height(); j++) {
        x = j % imageSize.width();
        y = j / imageSize.width();

        //Compute the value to compare to the threshold
        double w_odRunningTotal(0.0), w_odVal(0.0);
        int weightValsSize = 3;
        //If the source ColorModel is Grayscale, there is only one channel to consider
        if ((sourceColorModel == ColorModel::Grayscale) || (numSourceChannels == 1)) {

        }
        else {

        }

        pick up here!!!

        w_odVal = w_odRunningTotal / weightDenominator;


        //Loop over the number of channels that should contribute to the comparison value
        for (int ch = 0; ch < 3; ch++) {
            //Get the actual channel in the source image that should be accessed
            int sourceChannelNum = translateSourceToOutputChannels.at(ch);
            if (sourceChannelNum >= numSourceChannels) {
                assert(false && "Out of range index value");
            }
            w_odRunningTotal += m_weightVals.at(ch)
                * converter->LookupRGBtoOD(static_cast<int>((source[nextIndex]).as<s32>()));




                //    int weightValsSize = static_cast<int>(m_weightVals.size());
    //    int endNum = (numChannels < weightValsSize) ? numChannels : weightValsSize;
    //    for (int i = 0; i < endNum; i++) {
    //        int nextIndex = indices.at(i);
    //        if (nextIndex >= numElements) {
    //            assert(false && "Out of range index value");
    //        }
    //        w_odRunningTotal += m_weightVals.at(i)
    //            * converter->LookupRGBtoOD(static_cast<int>((source[nextIndex]).as<s32>()));


        }





    //    for (int comp = 0; comp < numChannels; comp++) {
    //        buffer->setValue(x, y, comp, source.at(x, y, comp));
    //    }




        //Set the last element of each output pixel
        buffer->setValue(x, y, 3, outputScaleMax);
    }//end loop over all pixels




    ////Loop through all pixels in the source
    //for (int px = 0; px < numPixels; px++) {
    //    //Create a set of indices corresponding to the elements of a pixel, however many there are
    //    std::vector<int> indices;
    //    for (int ch = 0; ch < 3; ch++) {
    //        if (pixelOrder == PixelOrder::Interleaved) {
    //            //RGB RGB RGB ... (if numChannels=3)
    //            indices.push_back(px*numChannels + ch);
    //        }
    //        else if (pixelOrder == PixelOrder::Planar) {
    //            //RRR... GGG... BBB...
    //            indices.push_back(ch*numPixels + px);
    //        }
    //        else {
    //            //Invalid value of pixelOrder
    //            assert(false && "Invalid PixelOrder defined");
    //        }
    //    }
    //    double w_odRunningTotal(0.0), w_odVal(0.0);
    //    int weightValsSize = static_cast<int>(m_weightVals.size());
    //    int endNum = (numChannels < weightValsSize) ? numChannels : weightValsSize;
    //    for (int i = 0; i < endNum; i++) {
    //        int nextIndex = indices.at(i);
    //        if (nextIndex >= numElements) {
    //            assert(false && "Out of range index value");
    //        }
    //        w_odRunningTotal += m_weightVals.at(i)
    //            * converter->LookupRGBtoOD(static_cast<int>((source[nextIndex]).as<s32>()));
    //    }
    //    w_odVal = w_odRunningTotal / weightDenominator;
    //    
    //    switch (m_behavior) {
    //    case RETAIN_LOWER_OD:
    //        if (w_odVal <= m_odThreshVal) {
    //            for (auto ni = indices.begin(); ni != indices.end(); ++ni) {
    //                buffer->setValue(*ni, source[*ni]);
    //            }
    //        }
    //        break;
    //    case RETAIN_HIGHER_OD:
    //        if (w_odVal >= m_odThreshVal) {
    //            for (auto ni = indices.begin(); ni != indices.end(); ++ni) {
    //                buffer->setValue(*ni, source[*ni]);
    //            }
    //        }
    //        break;
    //    default:
    //        assert(false && "Invalid Behavior defined");
    //    }
    //}//end for px





    return *buffer;
}//end doProcessData

const ColorSpace& ODThresholdKernel::doGetColorSpace() const {
    return OutputColor;
}

} // namespace tile

} // namespace image
} // namespace sedeen

