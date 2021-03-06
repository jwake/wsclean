#ifndef MORESANE_H
#define MORESANE_H

#include <string>

#include "deconvolutionalgorithm.h"
#include "imageset.h"

#include "../wsclean/imagebufferallocator.h"

class MoreSane : public DeconvolutionAlgorithm
{
	public:
		MoreSane(const std::string& moreSaneLocation, const std::string& moresaneArguments, 
		         const ao::uvector<double> &moresaneSigmaLevels, const std::string &prefixName,
						 ImageBufferAllocator& allocator, class FFTWManager& fftwManager) :
			_moresaneLocation(moreSaneLocation), _moresaneArguments(moresaneArguments), 
			_moresaneSigmaLevels(moresaneSigmaLevels), _prefixName(prefixName),
			_allocator(&allocator),
			_fftwManager(fftwManager)
		{ }
		
		virtual double ExecuteMajorIteration(ImageSet& dataImage, ImageSet& modelImage, const ao::uvector<const double*>& psfImages, size_t width, size_t height, bool& reachedMajorThreshold) final override;
		
		virtual std::unique_ptr<DeconvolutionAlgorithm> Clone() const final override
		{
			return std::unique_ptr<DeconvolutionAlgorithm>(new MoreSane(*this));
		}
		
		void ExecuteMajorIteration(double* dataImage, double* modelImage, const double* psfImage, size_t width, size_t height);
	private:
		const std::string _moresaneLocation, _moresaneArguments;

		const ao::uvector<double> _moresaneSigmaLevels;
		const std::string _prefixName;
		
		ImageBufferAllocator* _allocator;
		class FFTWManager& _fftwManager;
};

#endif
