#ifndef PROPOSALFUNCTIONS_H
#define PROPOSALFUNCTIONS_H
#include "bayesship/bayesshipSampler.h"
#include "bayesship/dataUtilities.h"
#include <vector>
#include <string>
#include <armadillo>

/*! \file 
 *
 * # Header file for general proposal functions
 */

#ifdef _MLPACK
#include <mlpack/core.hpp>
#include <mlpack/methods/kde/kde.hpp>
#endif

namespace bayesship{

//class proposal
//{
//public:
//	proposal();
//	virtual ~proposal();
//	virtual void propose(positionInfo *current, positionInfo *proposed, int chainID,double *MHRatioModifications)
//	{
//		proposed->updatePosition(current);
//		return;	
//	};
//	/*! \brief proposalFn write checkpoint function typedef 
//	 *
//	 * Specifies the form of proposal function write checkpoint functions.
//	 *
//	 * These functions are called when writing out checkpoint files. It's up to the user whether and how these files should be written.
//	 *
//	 * These functions are only necessary if the proposal saves data or optimizes itself in some way. If not necessary, (void *) can be passed instead.
//	 *
//	 * The write and read checkpoint functions *should be compatible*. That is, the file naming should be consistent between the two, in such a way the files being written out can be read in without external information.
//	 * 
//	 * */
//	virtual void writeCheckpoint(std::string outputDirectory, std::string runMoniker )
//	{
//		return ;
//	};
//	/*! \brief proposalFn read checkpoint function typedef 
//	 *
//	 * Specifies the form of proposal function read checkpoint functions.
//	 *
//	 * These functions are called when reading in checkpoint files. It's up to the user whether and how these files should be written.
//	 *
//	 * These functions are only necessary if the proposal saves data or optimizes itself in some way. If not necessary, (void *) can be passed instead.
//	 *
//	 * The write and read checkpoint functions *should be compatible*. That is, the file naming should be consistent between the two, in such a way the files being written out can be read in without external information.
//	 * 
//	 * */
//	virtual void loadCheckpoint( std::string inputDirectory, std::string runMoniker)
//	{
//		return ;
//	};
//
//};


//###################################################################
//###################################################################

typedef void(*blockFisherCalculation)(
	positionInfo *pos,
	double **fisher,
	std::vector<int> ids,
	void *parameters
	);


class blockFisherProposal: public proposal
{
public:
	/*! Number of chains*/
	int chainN;
	/*! Maximum dimension of the space*/
	int maxDim;
	/*! Array of step widths (standard deviations) of Gaussian proposals for each dimension for each chain*/
	double ****Fisher=nullptr;
	double ***FisherEigenVals = nullptr;
	double ****FisherEigenVecs = nullptr;
	bool **noFisher=nullptr;
	
	int **FisherAttemptsSinceLastUpdate=nullptr;
	int updateFreq = 200;

	std::vector<std::vector<int>> blocks;
	std::vector<double> blockProb;
	std::vector<double> blockProbBoundaries;

	void **parameters=nullptr;

	bayesshipSampler *sampler=nullptr;
	
	blockFisherCalculation fisherCalc;


	blockFisherProposal(int chainN, int maxDim, blockFisherCalculation fisherCalc, void **parameters, int updateFreq,bayesshipSampler *sampler, std::vector<std::vector<int>> blocks,std::vector<double> blockProb);
	virtual ~blockFisherProposal();
	virtual void propose(positionInfo *current, positionInfo *proposed, int chainID,int stepID,double *MHRatioModifications);

};



//###################################################################
//###################################################################

typedef void(*FisherCalculation)(
	positionInfo *pos,
	double **fisher,
	void *parameters
	);


class fisherProposal: public proposal
{
public:
	/*! Number of chains*/
	int chainN;
	/*! Maximum dimension of the space*/
	int maxDim;
	/*! Array of step widths (standard deviations) of Gaussian proposals for each dimension for each chain*/
	double ***Fisher=nullptr;
	double **FisherEigenVals = nullptr;
	double ***FisherEigenVecs = nullptr;
	bool *noFisher=nullptr;
	
	int *FisherAttemptsSinceLastUpdate=nullptr;
	int updateFreq = 200;

	void **parameters=nullptr;

	bayesshipSampler *sampler=nullptr;
	
	FisherCalculation fisherCalc;

	fisherProposal(int chainN, int maxDim, FisherCalculation fisherCalc, void **parameters, int updateFreq,bayesshipSampler *sampler);
	virtual ~fisherProposal();
	virtual void propose(positionInfo *current, positionInfo *proposed, int chainID,int stepID,double *MHRatioModifications);

};



//###################################################################
//###################################################################


//###################################################################
//###################################################################

class gaussianProposal: public proposal
{
public:
	/*! Vector of random number generators*/
	gsl_rng **r=nullptr;
	/*! Number of chains*/
	int chainN;
	/*! Maximum dimension of the space*/
	int maxDim;
	/*! Array of step widths (standard deviations) of Gaussian proposals for each dimension for each chain*/
	double **gaussWidths=nullptr;
	/*! Track last step ID for updating widths*/
	int *previousDimID=nullptr;
	/*! Track what the last acceptance was for updating widths*/
	int *previousAccepts=nullptr;
	bayesshipSampler *sampler=nullptr;

	gaussianProposal(int chainN, int maxDim , bayesshipSampler *sampler,int seed=1);
	virtual ~gaussianProposal();
	virtual void propose(positionInfo *current, positionInfo *proposed, int chainID,int stepID,double *MHRatioModifications);
	virtual void writeCheckpoint(std::string outputDirectory , std::string runMoniker);
	virtual void loadCheckpoint( std::string inputDirectory, std::string runMoniker);

};


//###################################################################
//###################################################################

class KDEProposal: public proposal
{
public:
	/*! Vector of random number generators*/
	gsl_rng **r=nullptr;
	/*! Number of chains*/
	int chainN;
	/*! Maximum dimension of the space*/
	int maxDim;
	/*! Current step number in storage*/
	int *stepNumber=nullptr;
	/*! Current size of the storage for the KDE structure*/
	int *currentStorageSize=nullptr;
	/*! Last ID that was harvested from a samplerData structure*/
	int *lastUpdatePositionID=nullptr;
	/*! Continuously updated variances of the different distributions*/
	double ***runningCov=nullptr;
	double ***runningCovCholeskyDecomp=nullptr;
	double **runningSTD=nullptr;
	/*! Continuously updated means of the different distributions*/
	double **runningMean=nullptr;
	/*Bandwidths of the various kdes -- using Scott factor*/
	double *bandwidth = nullptr;
	/*! Pointer of the current samplerData object -- if this changes, we can restart counters (Does NOT erase old samples)*/
	samplerData **currentData = nullptr;
	
	bayesshipSampler *sampler;

	/*! Previous samples stored for KDE usage*/
	/*! Could make this a new structure as a linked list to have variable size, but not now*/
	positionInfo ***storedSamples = nullptr;
	
	/*! The number of samples to skip between storing samples in storage*/
	int updateInterval;

	int *drawCt = nullptr;

	/*! Batch size to allocate memory for the storage of past samples*/
	int batchSize;

	bool RJ;

	bool useMLPack=false;

	/*! Batch size to use for each training of the KDE*/
	int KDETrainingBatchSize;
	/*! IDs used for the latest training*/
	std::vector<int> *trainingIDs=nullptr;

#ifdef _MLPACK
	mlpack::kde::KDE<mlpack::kernel::GaussianKernel,mlpack::metric::EuclideanDistance,arma::mat, mlpack::tree::KDTree> **kde=nullptr;
#endif
	KDEProposal(
		int chainN, /**< Number of chains in the ensemble*/
		int maxDim, /**< Maximum dimension of the space*/
		bayesshipSampler *sampler,
		bool RJ=false,
		int batchSize = 5000,/**< Batch size to allocate memory for data points */
		int KDETrainingBatchSize= 1000,/**< Batch size to use for KDE training/eval ; -1 means full*/
		int updateInterval = 5,/**< number of steps to take before storing a sample*/
		int seed=1 /**< Seed to use for initiating random numbers*/
	);
	virtual ~KDEProposal();
	virtual void propose(positionInfo *current, positionInfo *proposed, int chainID,int stepID,double *MHRatioModifications);
	void updateCov( int chainID);
	//void updateVar( int chainID);
	int trainKDE(int chainID );
	int trainKDEMLPACK(int chainID );
	int trainKDECustom(int chainID );
	void updateStorageSize(int chainID);
	void reset(int chainID);
	double evalKDEMLPACK(positionInfo *position,int chainID); 
	double evalKDECustom(positionInfo *position,int chainID); 
	double evalKDE(positionInfo *position,int chainID); 

};


int KDEDraw(positionInfo *sampleLocation, double **cov, positionInfo *output);

//###################################################################
//###################################################################

class differentialEvolutionProposal: public proposal
{
public:
	bayesshipSampler *sampler=nullptr;
	differentialEvolutionProposal(bayesshipSampler *sampler);
	virtual ~differentialEvolutionProposal(){return;};
	virtual void propose(positionInfo *current, positionInfo *proposed, int chainID,int stepID,double *MHRatioModifications);

};
//###################################################################
//###################################################################

class blockDifferentialEvolutionProposal: public proposal
{
public:
	std::vector<std::vector<int>> blocks;
	std::vector<double> blockProb;
	std::vector<double> blockProbBoundaries;
	bayesshipSampler *sampler=nullptr;
	blockDifferentialEvolutionProposal(bayesshipSampler *sampler, std::vector<std::vector<int>> blocks, std::vector<double> blockProb);
	virtual ~blockDifferentialEvolutionProposal(){return;};
	virtual void propose(positionInfo *current, positionInfo *proposed, int chainID,int stepID,double *MHRatioModifications);

};


//###################################################################
//###################################################################
/*Reversible Jump proposal -- layer on modifications sequentially with creation probability alpha and destruction probability 1-alpha*/

class sequentialLayerRJProposal: public proposal
{
public:
	double alpha= .5;
	bayesshipSampler *sampler;
	sequentialLayerRJProposal(bayesshipSampler *sampler,double alpha=.5 );
	virtual ~sequentialLayerRJProposal(){return;};
	virtual void propose(positionInfo *current, positionInfo *proposed, int chainID,int stepID,double *MHRatioModifications);

};
//###################################################################
//###################################################################
/*Reversible Jump proposal -- randomly add/subtract modifications with creation probability alpha and destruction probability 1-alpha*/

class randomLayerRJProposal: public proposal
{
public:
	double alpha= .5;
	bayesshipSampler *sampler;
	randomLayerRJProposal(bayesshipSampler *sampler,double alpha=.5 );
	virtual ~randomLayerRJProposal(){return;};
	virtual void propose(positionInfo *current, positionInfo *proposed, int chainID,int stepID,double *MHRatioModifications);

};


//###################################################################
//###################################################################

class jointKDEProposal: public proposal
{
public:
	int additionalThreads = 1;
	int ensembleSize ;
	//std::vector<std::mutex> historyMutexVec;
	//std::vector<std::mutex> historyMutexVec;
	jointKDEProposal(int ensembleSize, int ,int threads=1);
	virtual ~jointKDEProposal();
	virtual void propose(positionInfo *current, positionInfo *proposed, int chainID,int stepID,double *MHRatioModifications);
	virtual void writeCheckpoint(std::string outputDirectory , std::string runMoniker);
	virtual void loadCheckpoint( std::string inputDirectory, std::string runMoniker);

};


//###################################################################
//###################################################################
class GMMProposal: public proposal
{
public:
	/*! Number of chains*/
	int chainN;
	/*! Maximum dimension of the space*/
	int maxDim;
	/*! Current step number in storage*/
	int *stepNumber=nullptr;
	/*! Last ID that was harvested from a samplerData structure*/
	int *lastUpdatePositionID=nullptr;
	/*! Pointer of the current samplerData object -- if this changes, we can restart counters (Does NOT erase old samples)*/
	samplerData **currentData = nullptr;
	
	bayesshipSampler *sampler;

	/*! The number of samples to skip between storing samples in storage*/
	int updateInterval;

	bool RJ;
	 
	int gaussians;
	int km_iter;
	int em_iter;
	double var_floor;

	arma::gmm_full *models=nullptr;

	bool *primed=nullptr;


	std::vector<std::vector<int>> blocks;
	std::vector<double> blockProb;
	std::vector<double> blockProbBoundaries;

	GMMProposal(
		int chainN, /**< Number of chains in the ensemble*/
		int maxDim, /**< Maximum dimension of the space*/
		bayesshipSampler *sampler,
		std::vector<std::vector<int>> blocks, 
		std::vector<double> blockProb,
		int gaussians=10,/**< Number of gaussians to use*/
		int km_iter=10,/**< Number of gaussians to use*/
		int em_iter=10,/**< Number of gaussians to use*/
		double var_floor=1e-10,/**< Number of gaussians to use*/
		bool RJ=false,
		int updateInterval = 1000/**< number of steps to take before storing a sample*/
	);
	virtual ~GMMProposal();
	virtual void propose(positionInfo *current, positionInfo *proposed, int chainID,int stepID,double *MHRatioModifications);
	bool train(int chainID);

};

}

#endif
