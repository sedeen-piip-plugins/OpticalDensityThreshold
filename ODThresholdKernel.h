/*=============================================================================
 *
 *  Copyright (c) 2021 Sunnybrook Research Institute
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

#ifndef SEDEEN_SRC_IMAGE_FILTER_KERNELS_ODTHRESHOLD_H
#define SEDEEN_SRC_IMAGE_FILTER_KERNELS_ODTHRESHOLD_H

#include "image/filter/Kernel.h"

#include "ODConversion.h"

#include <array>

namespace sedeen {

class ColorSpace;
class Graphic;
class Rect;
class Size;

namespace image {
namespace tile {

class Factory;	

/// \ingroup algorithm_kernels
//
/// Generates a binary mask by applying a threshold (based on optical density) to an image
//
/// \copydetails ODThresholdKernel::doProcessData()
class PATHCORE_IMAGE_API ODThresholdKernel : public Kernel {
public:
    /// Threshold behavior
    enum Behavior {
        /// Create a binary mask for pixels with weighted optical density below the threshold value
        RETAIN_LOWER_OD,
        /// Create a binary mask for pixels with weighted optical density at or above the threshold value
        RETAIN_HIGHER_OD,
        /// Do nothing
        NO_ACTION
    };
    
    /// Creates an optical density thresholding Kernel 
    //
    /// \param ODThreshVal
    /// threshold parameter
    //
    /// \param behavior
    /// Threshold output inclusion criteria
    //
    /// \param weights
    /// An array of three values to define how to combine OD_R, OD_G, OD_B into a single OD value
    //
    explicit ODThresholdKernel(double ODThreshVal, Behavior behavior, 
        std::array<double, 3> weights = { 1.0,1.0,1.0 });

    virtual ~ODThresholdKernel();

    /// Set the OD threshold value of the kernel
    /// \param ODThreshVal
    /// The ODThreshVal parameter
    void setODThreshold(double v);

    /// Set the behavior of the kernel
    /// \param t
    /// The threshold behavior
    void setBehavior(Behavior t);

    /// Set the optical density weights of the kernel
    /// \param w
    /// The weights to apply to OD_R, OD_G, OD_B to get a single OD value
    void setWeights(std::array<double,3> w);

private:
	/// \cond INTERNAL

    /// Applies the kernel to \p source
    ///
    /// The output of this Kernel is the same color space as the source image,
    /// with the same colors as the source when retained, or black (i.e. 0) when not retained by the threshold.
    /// This depends on the threshold value, Behavior, and OD weights.
    virtual RawImage doProcessData(const RawImage &source);

    ///Return the output ColorSpace of this kernel, which is fixed as RGBA
    virtual const ColorSpace& doGetColorSpace() const;

    double m_odThreshVal;
    ODThresholdKernel::Behavior m_behavior;
    std::array<double, 3> m_weightVals;

    /// \endcond
};

} // namespace tile

} // namespace image
} // namespace sedeen
#endif

