#include "deconvolution.h"

#include "joinedclean.h"
#include "simpleclean.h"
#include "moresane.h"
#include "fastmultiscaleclean.h"
#include "iuwtdeconvolution.h"

#include "../multiscale/multiscalealgorithm.h"

#include "../casamaskreader.h"
#include "../wsclean/imagingtable.h"

Deconvolution::Deconvolution() :
	_threshold(0.0), _gain(0.1), _mGain(1.0),
	_nIter(0),
	_allowNegative(true), 
	_stopOnNegative(false),
	_multiscale(false), _fastMultiscale(false),
	_multiscaleThresholdBias(0.7), _multiscaleScaleBias(0.6),
	_cleanBorderRatio(0.05),
	_fitsMask(), _casaMask(),
	_useMoreSane(false),
	_useIUWT(false),
	_moreSaneLocation(), _moreSaneArgs(),
	_spectralFittingMode(NoSpectralFitting),
	_spectralFittingTerms(0),
	_requestedDeconvolutionChannelCount(0)
{
}

Deconvolution::~Deconvolution()
{
	FreeDeconvolutionAlgorithms();
}

void Deconvolution::Perform(const class ImagingTable& groupTable, bool& reachedMajorThreshold, size_t majorIterationNr)
{
	Logger::Info.Flush();
	Logger::Info << " == Cleaning (" << majorIterationNr << ") ==\n";
	
	_imageAllocator->FreeUnused();
	DynamicSet
		residualSet(&groupTable, *_imageAllocator, _requestedDeconvolutionChannelCount, _imgWidth, _imgHeight),
		modelSet(&groupTable, *_imageAllocator, _requestedDeconvolutionChannelCount, _imgWidth, _imgHeight);
		
	residualSet.LoadAndAverage(*_residualImages);
	modelSet.LoadAndAverage(*_modelImages);
	
	std::vector<ao::uvector<double>> psfVecs(groupTable.SquaredGroupCount());
	residualSet.LoadAndAveragePSFs(*_psfImages, psfVecs, _psfPolarization);
	
	ao::uvector<const double*> psfs(groupTable.SquaredGroupCount());
	for(size_t i=0; i!=psfVecs.size(); ++i)
		psfs[i] = psfVecs[i].data();
	
	if(_useIUWT || _multiscale || _useMoreSane)
	{
		UntypedDeconvolutionAlgorithm& algorithm =
			static_cast<UntypedDeconvolutionAlgorithm&>(*_cleanAlgorithm);
		algorithm.ExecuteMajorIteration(residualSet, modelSet, psfs, _imgWidth, _imgHeight, reachedMajorThreshold);
	}
	else if(_summedCount != 1)
	{
		if(_squaredCount == 4)
			performJoinedPolFreqClean<4>(residualSet, modelSet, psfs, reachedMajorThreshold, majorIterationNr);
		else if(_squaredCount == 2)
			performJoinedPolFreqClean<2>(residualSet, modelSet, psfs, reachedMajorThreshold, majorIterationNr);
		else // if(_squaredCount == 1)
			performJoinedPolFreqClean<1>(residualSet, modelSet, psfs, reachedMajorThreshold, majorIterationNr);
	}
	else if(_squaredCount != 1) {
		if(_squaredCount == 4)
			performJoinedPolClean<4>(residualSet, modelSet, psfs, reachedMajorThreshold, majorIterationNr);
		else // if(_squaredCount == 2)
			performJoinedPolClean<2>(residualSet, modelSet, psfs, reachedMajorThreshold, majorIterationNr);
	}
	else {
		performSimpleClean(residualSet, modelSet, psfs, reachedMajorThreshold, majorIterationNr);
	}
	
	residualSet.AssignAndStore(*_residualImages);
	
	SpectralFitter fitter(_spectralFittingMode, _spectralFittingTerms);
	modelSet.InterpolateAndStore(*_modelImages, _cleanAlgorithm->Fitter());
}

void Deconvolution::performSimpleClean(DynamicSet& residual, DynamicSet& model, const ao::uvector<const double*>& psfs, bool& reachedMajorThreshold, size_t majorIterationNr)
{
	deconvolution::SingleImageSet
		residualImage(residual.Release(0), *_imageAllocator),
		modelImage(model.Release(0), *_imageAllocator);
		
	TypedDeconvolutionAlgorithm<deconvolution::SingleImageSet>& tAlgorithm =
		static_cast<TypedDeconvolutionAlgorithm<deconvolution::SingleImageSet>&>(*_cleanAlgorithm);
	tAlgorithm.ExecuteMajorIteration(residualImage, modelImage, psfs, _imgWidth, _imgHeight, reachedMajorThreshold);
	
	residualImage.Transfer(residual);
	modelImage.Transfer(model);
}

template<size_t PolCount>
void Deconvolution::performJoinedPolClean(DynamicSet& residual, DynamicSet& model, const ao::uvector<const double*>& psfs, bool& reachedMajorThreshold, size_t majorIterationNr)
{
	typename JoinedClean<deconvolution::PolarizedImageSet<PolCount>>::ImageSet
		modelSet(model, *_imageAllocator),
		residualSet(residual, *_imageAllocator);
		
	static_cast<TypedDeconvolutionAlgorithm<deconvolution::PolarizedImageSet<PolCount>>&>(*_cleanAlgorithm).ExecuteMajorIteration(residualSet, modelSet, psfs, _imgWidth, _imgHeight, reachedMajorThreshold);
	
	modelSet.Transfer(model, 0);
	residualSet.Transfer(residual, 0);
}

template<size_t PolCount>
void Deconvolution::performJoinedPolFreqClean(DynamicSet& residual, DynamicSet& model, const ao::uvector<const double*>& psfs, bool& reachedMajorThreshold, size_t majorIterationNr)
{
	typename JoinedClean<deconvolution::MultiImageSet<deconvolution::PolarizedImageSet<PolCount>>>::ImageSet
		modelSet(model, model.ChannelsInDeconvolution(), *_imageAllocator),
		residualSet(residual, residual.ChannelsInDeconvolution(), *_imageAllocator);
	static_cast<TypedDeconvolutionAlgorithm<deconvolution::MultiImageSet<deconvolution::PolarizedImageSet<PolCount>>>&>(*_cleanAlgorithm).ExecuteMajorIteration(residualSet, modelSet, psfs, _imgWidth, _imgHeight, reachedMajorThreshold);
	
	modelSet.Transfer(model);
	residualSet.Transfer(residual);
}

void Deconvolution::FreeDeconvolutionAlgorithms()
{
	_cleanAlgorithm.reset();
}

void Deconvolution::InitializeDeconvolutionAlgorithm(const ImagingTable& groupTable, PolarizationEnum psfPolarization, class ImageBufferAllocator* imageAllocator, size_t imgWidth, size_t imgHeight, double pixelScaleX, double pixelScaleY, size_t outputChannels, double beamSize, size_t threadCount)
{
	_imageAllocator = imageAllocator;
	_imgWidth = imgWidth;
	_imgHeight = imgHeight;
	_psfPolarization = psfPolarization;
	FreeDeconvolutionAlgorithms();
	
	_summedCount = groupTable.SquaredGroupCount();
	if(_summedCount == 0)
		throw std::runtime_error("Nothing to clean");
	
	ImagingTable firstSquaredGroup = groupTable.GetSquaredGroup(0);
	_squaredCount = firstSquaredGroup.EntryCount();
	_polarizations.clear();
	for(size_t p=0; p!=_squaredCount; ++p)
	{
		if(_polarizations.count(firstSquaredGroup[p].polarization) != 0)
			throw std::runtime_error("Two equal polarizations were given to the deconvolution algorithm within a single polarized group");
		else
			_polarizations.insert(firstSquaredGroup[p].polarization);
	}
	
	if(_useMoreSane)
	{
		_cleanAlgorithm.reset(new MoreSane(_moreSaneLocation, _moreSaneArgs, _moreSaneSigmaLevels, _prefixName, *_imageAllocator));
	}
	else if(_useIUWT)
	{
		_cleanAlgorithm.reset(new IUWTDeconvolution());
	}
	else if(_multiscale)
	{
		_cleanAlgorithm.reset(new MultiScaleAlgorithm(*_imageAllocator, beamSize, pixelScaleX, pixelScaleY));
	}
	else if(_squaredCount != 1)
	{
		if(_squaredCount != 2 && _squaredCount != 4)
			throw std::runtime_error("Joined polarization cleaning was requested, but can't find a compatible set of 2 or 4 pols to clean");
		bool hasXY = _polarizations.count(Polarization::XY)!=0;
		bool hasYX = _polarizations.count(Polarization::YX)!=0;
		if((hasXY && !hasYX) || (hasYX && !hasXY))
			throw std::runtime_error("Cannot jointly clean polarization XY or YX without cleaning both.");
			
		if(_summedCount != 1)
		{
			if(_fastMultiscale)
			{
				if(_squaredCount == 4)
				{
					_cleanAlgorithm.reset(
					new FastMultiScaleClean
					<deconvolution::MultiImageSet
					<deconvolution::PolarizedImageSet<4>>>(beamSize, pixelScaleX, pixelScaleY));
				}
				else {
					_cleanAlgorithm.reset(
					new FastMultiScaleClean
					<deconvolution::MultiImageSet<deconvolution::PolarizedImageSet<2>>>(beamSize, pixelScaleX, pixelScaleY));
				}
			}
			else {
				if(_squaredCount == 4)
					_cleanAlgorithm.reset(new JoinedClean<deconvolution::MultiImageSet<deconvolution::PolarizedImageSet<4>>>());
				else
					_cleanAlgorithm.reset(new JoinedClean<deconvolution::MultiImageSet<deconvolution::PolarizedImageSet<2>>>());
			}
		}
		else {
			if(_fastMultiscale)
			{
				if(_squaredCount == 4)
					_cleanAlgorithm.reset(new FastMultiScaleClean<deconvolution::PolarizedImageSet<4>>(beamSize, pixelScaleX, pixelScaleY));
				else
					_cleanAlgorithm.reset(new FastMultiScaleClean<deconvolution::PolarizedImageSet<2>>(beamSize, pixelScaleX, pixelScaleY));
			}
			else
			{
				if(_squaredCount == 4)
					_cleanAlgorithm.reset(new JoinedClean<deconvolution::PolarizedImageSet<4>>());
				else
					_cleanAlgorithm.reset(new JoinedClean<deconvolution::PolarizedImageSet<2>>());
			}
		}
	}
	else { // squaredCount == 1
		if(_summedCount != 1)
		{
			if(_fastMultiscale)
				_cleanAlgorithm.reset(new FastMultiScaleClean<deconvolution::MultiImageSet<deconvolution::SingleImageSet>>(beamSize, pixelScaleX, pixelScaleY));
			else
				_cleanAlgorithm.reset(new JoinedClean<deconvolution::MultiImageSet<deconvolution::SingleImageSet>>());
		}
		else {
			if(_fastMultiscale)
				_cleanAlgorithm.reset(new FastMultiScaleClean<deconvolution::SingleImageSet>(beamSize, pixelScaleX, pixelScaleY));
			else
				_cleanAlgorithm.reset(new SimpleClean());
		}
	}
	
	_cleanAlgorithm->SetMaxNIter(_nIter);
	_cleanAlgorithm->SetThreshold(_threshold);
	_cleanAlgorithm->SetGain(_gain);
	_cleanAlgorithm->SetMGain(_mGain);
	_cleanAlgorithm->SetCleanBorderRatio(_cleanBorderRatio);
	_cleanAlgorithm->SetAllowNegativeComponents(_allowNegative);
	_cleanAlgorithm->SetStopOnNegativeComponents(_stopOnNegative);
	_cleanAlgorithm->SetThreadCount(threadCount);
	_cleanAlgorithm->SetMultiscaleScaleBias(_multiscaleScaleBias);
	_cleanAlgorithm->SetMultiscaleThresholdBias(_multiscaleThresholdBias);
	_cleanAlgorithm->SetSpectralFittingMode(_spectralFittingMode, _spectralFittingTerms);
	
	ao::uvector<double> frequencies;
	calculateDeconvolutionFrequencies(groupTable, frequencies);
	_cleanAlgorithm->InitializeFrequencies(frequencies);
	
	if(!_fitsMask.empty())
	{
		if(_cleanMask.empty())
		{
			Logger::Info << "Reading mask '" << _fitsMask << "'...\n";
			FitsReader maskReader(_fitsMask);
			if(maskReader.ImageWidth() != _imgWidth || maskReader.ImageHeight() != _imgHeight)
				throw std::runtime_error("Specified Fits file mask did not have same dimensions as output image!");
			ao::uvector<float> maskData(_imgWidth*_imgHeight);
			maskReader.Read(maskData.data());
			_cleanMask.assign(_imgWidth*_imgHeight, false);
			for(size_t i=0; i!=_imgWidth*_imgHeight; ++i)
				_cleanMask[i] = maskData[i]!=0.0;
		}
		_cleanAlgorithm->SetCleanMask(_cleanMask.data());
	}
	else if(!_casaMask.empty())
	{
		if(_cleanMask.empty())
		{
			Logger::Info << "Reading CASA mask '" << _casaMask << "'...\n";
			_cleanMask.assign(_imgWidth*_imgHeight, false);
			CasaMaskReader maskReader(_casaMask);
			if(maskReader.Width() != _imgWidth || maskReader.Height() != _imgHeight)
				throw std::runtime_error("Specified CASA mask did not have same dimensions as output image!");
			maskReader.Read(_cleanMask.data());
		}
		_cleanAlgorithm->SetCleanMask(_cleanMask.data());
	}
}

void Deconvolution::calculateDeconvolutionFrequencies(const ImagingTable& groupTable, ao::uvector<double>& frequencies)
{
	size_t deconvolutionChannels = _requestedDeconvolutionChannelCount;
	if(deconvolutionChannels == 0) deconvolutionChannels = _summedCount;
	frequencies.assign(deconvolutionChannels, 0.0);
	ao::uvector<size_t> weights(deconvolutionChannels, 0);
	for(size_t i=0; i!=_summedCount; ++i)
	{
		double freq = groupTable.GetSquaredGroup(i)[0].CentralFrequency();
		size_t deconvolutionChannel = i * deconvolutionChannels / _summedCount;
		frequencies[deconvolutionChannel] += freq;
		weights[deconvolutionChannel]++;
	}
	for(size_t i=0; i!=deconvolutionChannels; ++i)
		frequencies[i] /= weights[i];
}