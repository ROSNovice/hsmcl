/**
# ##############################################################################
#  Copyright (c) 2021- University of Bonn                                      #
#  All rights reserved.                                                        #
#                                                                              #
#  Author: Nicky Zimmerman                                                     #
#                                                                              #
#  File: NMCLUnitTests.cpp             		                           		   #
# ##############################################################################
**/



#include "gtest/gtest.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <math.h>
#include <string>
#include <fstream>
#include <chrono>
#include <stdlib.h>
#include <string>



#include "Utils.h"
#include "Lidar2D.h"
#include "OptiTrack.h"

#include "BeamEnd.h"
#include "Analysis.h"
#include "MixedFSR.h"
#include "SetStatistics.h"
#include "Camera.h"
#include "PlaceRecognition.h"
#include "FloorMap.h"
#include "ReNMCL.h"
#include <nlohmann/json.hpp>
#include "NMCLFactory.h"
#include "LidarData.h"
#include "SemanticLikelihood.h"
#include "SemanticVisibility.h"
#include "ParticleFilter.h"

std::string dataPath = PROJECT_TEST_DATA_DIR + std::string("/8/");
std::string testPath = PROJECT_TEST_DATA_DIR + std::string("/test/floor/");


TEST(TestSemanticVisibility, test1)
{
	std::string mapFolder = testPath + "SemMaps/";
	cv::Mat grid = cv::imread(testPath + "JMap.png");
	std::vector<std::string> classNames = {"sink", "door", "oven", "whiteboard", "table", "cardboard", "plant", "drawers", "sofa", "storage"};
	std::vector<float>  confidencees = {0.7, 0.5, 0.7, 0.7, 0.6, 0.7, 0.7, 0.7, 0.8, 0.7};
	
	GMap gmap(grid, Eigen::Vector3f(-13.9155, -24.94537, 0.0), 0.05);
	SemanticVisibility sv(std::make_shared<GMap>(gmap), 36, mapFolder, classNames, confidencees);
	
	std::vector<Particle> particles = {Particle(Eigen::Vector3f(1.3165115852616611, -7.790449181330476, -0.1909482910790089), 1.0)};
	
	std::vector<Eigen::Vector2f> poses = {Eigen::Vector2f(1.0, 0.4537924009538352), Eigen::Vector2f(1.0, -0.4631433462056238),
											Eigen::Vector2f(1.0, -0.15925215884000687), Eigen::Vector2f(1.0, -0.3750017464069384),
											Eigen::Vector2f(1.0, -0.1250479559330543)};

	std::vector<int> labels = {1, 4, 3, 7, 4};
	std::vector<float> conf = {0.8995144, 0.8903151, 0.88935226, 0.81773764, 0.8013637};
	SemanticData semData(labels, poses, conf);

	sv.ComputeWeights(particles, std::make_shared<SemanticData>(semData));
	ASSERT_GE(particles[0].weight, 0.95);
}



TEST(TestMixedFSR, test1) {
    
    Eigen::Vector3f p1 = Eigen::Vector3f(1.2, -2.5, 0.67);
	Eigen::Vector3f p2 = Eigen::Vector3f(-1.5, 4.3, -1.03);

	MixedFSR fsr = MixedFSR();
	Eigen::Vector3f u = fsr.Backward(p1, p2);
	Eigen::Vector3f p2_comp = fsr.Forward(p1, u);
	ASSERT_EQ(p2_comp, p2);
}

TEST(TestMixedFSR, test2) {

	MixedFSR fsr = MixedFSR();
	//test against know python result
	Eigen::Vector3f u2_gt = Eigen::Vector3f(-13.755235632332337, -3.971585665576746, 0.62);
	Eigen::Vector3f u2 = fsr.Backward(Eigen::Vector3f(13.6, -6.1, -0.33), Eigen::Vector3f(-0.7, -5.4, 0.29));
	ASSERT_NEAR(u2_gt(0), u2(0), 0.001);
	ASSERT_NEAR(u2_gt(1), u2(1), 0.001);
	ASSERT_NEAR(u2_gt(2), u2(2), 0.001);
}

TEST(TestMixedFSR, test3) {

	MixedFSR mfsr = MixedFSR();
	//float choose  = drand48();
	//test against know python result
	Eigen::Vector3f p_gt = Eigen::Vector3f(13.61489781, -6.21080639, -0.31);
	std::vector<float> commandWeights{0.5f, 0.5f};

	std::vector<Eigen::Vector3f> command{Eigen::Vector3f(0.05, -0.1, 0.02), Eigen::Vector3f(0.05, -0.1, 0.02)};
	Eigen::Vector3f noisy_p = mfsr.SampleMotion(Eigen::Vector3f(13.6, -6.1, -0.33), command, commandWeights, Eigen::Vector3f(0.0, 0.0, 0.0));
	ASSERT_NEAR(noisy_p(0), p_gt(0), 0.001);
	ASSERT_NEAR(noisy_p(1), p_gt(1), 0.001);
	ASSERT_NEAR(noisy_p(2), p_gt(2), 0.001);
	
}



TEST(TestPlaceRecognition, test1)
{
	std::vector<std::string> places = {"{Room 6 |", "[Rooné 3", "{Rooms 4", "Room 8", "+ Room 6§", "(Rooms 7"};
	std::vector<std::string> dict = {"NotValid", "Room 1", "Room 2","Room 3","Room 4","Room 5","Room 6","Room 7","Room 8","Room 9","Room 10"};
	PlaceRecognition pr = PlaceRecognition(dict, testPath + "TextMaps/");
	std::vector<int> res = pr.Match(places);

	ASSERT_EQ(4, res.size());
	ASSERT_EQ(6, res[0]);
	ASSERT_EQ(4, res[1]);
	ASSERT_EQ(8, res[2]);

}

TEST(TestPlaceRecognition, test2)
{
	std::vector<std::string> dict = {"NotValid", "Room 1", "Room 2","Room 3","Room 4","Room 5","Room 6","Room 7","Room 8","Room 9","Room 10"};
	PlaceRecognition placeRec = PlaceRecognition(dict, testPath + "TextMaps/");
	std::vector<std::string> places = {"Room2"};
	std::vector<int> matches = placeRec.Match(places);


	ASSERT_EQ(matches.size(), 1);
}

TEST(TestPlaceRecognition, test3)
{
    std::vector<std::string> dict = {"NotValid", "Room 1", "Room 2","Room 3","Room 4","Room 5","Room 6","Room 7","Room 8","Room 9","Room 10"};
	PlaceRecognition placeRec = PlaceRecognition(dict, testPath + "TextMaps/");

	std::vector<std::string> places = {"Room'1"};
	std::vector<int> matches = placeRec.Match(places);


	ASSERT_EQ(1, matches.size());
	ASSERT_EQ(1, matches[0]);
}

TEST(TestPlaceRecognition, test4)
{
    std::vector<std::string> dict = {"NotValid", "Room 1", "Room 2","Room 3","Room 4","Room 5","Room 6","Room 7","Room 8","Room 9","Room 10"};
	PlaceRecognition placeRec = PlaceRecognition(dict, testPath + "TextMaps/");

	std::vector<std::string> places = {"Room'10"};
	std::vector<int> detections = placeRec.Match(places);

	ASSERT_EQ(1, detections.size());
	ASSERT_EQ(10, detections[0]);
}

TEST(TestPlaceRecognition, test5)
{
	std::string jsonPath = testPath + "floor.config";
    using json = nlohmann::json;
    std::ifstream file(jsonPath);
    json config;
    file >> config;
    FloorMap fp(config, testPath);

    std::vector<std::string> dict = fp.GetRoomNames(); 
	PlaceRecognition placeRec = PlaceRecognition(dict, testPath + "TextMaps/");

	std::vector<std::string> places = {"Room'10"};
	std::vector<int> detections = placeRec.Match(places);

	ASSERT_EQ(1, detections.size());
	ASSERT_EQ(11, detections[0]);
}



TEST(TestParticleFilter, test1)
{
	std::string jsonPath = testPath + "floor.config";
    using json = nlohmann::json;
    std::ifstream file(jsonPath);
    json config;
    file >> config;
    FloorMap fp(config, testPath);

	ParticleFilter pf(std::make_shared<FloorMap>(fp));
	std::vector<Particle> particles;

	int n = 10;

	std::vector<Eigen::Vector3f> initGuesses{Eigen::Vector3f(0.1, 0.1 , 0), Eigen::Vector3f(-0.1, -0.1, 0)};
	Eigen::Matrix3d cov = Eigen::Matrix3d::Zero(3, 3);
	std::vector<Eigen::Matrix3d> covariances{cov, cov};

	pf.InitGaussian(particles, n, initGuesses, covariances);
	ASSERT_EQ(particles.size(), n * initGuesses.size());
	ASSERT_EQ(particles[0].pose, Eigen::Vector3f(0.1, 0.1 , 0));
	ASSERT_EQ(particles[n].pose, Eigen::Vector3f(-0.1, -0.1, 0));
}

TEST(TestParticleFilter, test2)
{
	std::string jsonPath = testPath + "floor.config";
    using json = nlohmann::json;
    std::ifstream file(jsonPath);
    json config;
    file >> config;
    FloorMap fp(config, testPath);

	ParticleFilter pf(std::make_shared<FloorMap>(fp));
	std::vector<Particle> particles;

	int n = 10;

	Eigen::Vector3f v1(0.05, 0.05 , 0);
	Eigen::Vector3f v2(-0.1, -0.1, 0);


	std::vector<Eigen::Vector3f> initGuesses{v1, v2};
	Eigen::Matrix3d cov;
	cov << 0.1, 0, 0, 0, 0.1, 0, 0, 0, 0.1;
	std::vector<Eigen::Matrix3d> covariances{cov, cov};

	pf.InitGaussian(particles, n, initGuesses, covariances);

	Eigen::Vector3f avg1 = Eigen::Vector3f::Zero();
	Eigen::Vector3f avg2 = Eigen::Vector3f::Zero();
	for(int i = 0; i < n; ++i) avg1 += particles[i].pose;

	for(int i = n; i < 2 * n; ++i) avg2 += particles[i].pose;

	avg1 = avg1 / n;
	avg2 = avg2 / n;

	// std::cout << avg1 << std::endl;
	// std::cout << avg2 << std::endl;

	ASSERT_NEAR(avg1(0), v1(0) , 0.1);
	ASSERT_NEAR(avg1(1), v1(1) , 0.1);
	ASSERT_NEAR(avg1(2), v1(2), 0.1);
	ASSERT_NEAR(avg2(0), v2(0) , 0.1);
	ASSERT_NEAR(avg2(1), v2(1) , 0.1);
	ASSERT_NEAR(avg2(2), v2(2), 0.1);
	
}

TEST(TestParticleFilter, test3)
{
	std::string jsonPath = testPath + "floor.config";
    using json = nlohmann::json;
    std::ifstream file(jsonPath);
    json config;
    file >> config;
    FloorMap fp(config, testPath);

	ParticleFilter pf(std::make_shared<FloorMap>(fp));
	std::vector<Particle> particles;

	pf.InitUniform(particles, 1);
	std::cout << particles.size() << std::endl;
	ASSERT_EQ(particles.size(), 1);
}



TEST(TestBeamEnd, test1)
{
	GMap gmap = GMap(dataPath);
	BeamEnd be = BeamEnd(std::make_shared<GMap>(gmap), 8, 15, BeamEnd::Weighting(0));

	Eigen::Vector3f p3d0_gt = Eigen::Vector3f(0.33675906, -0.84122932,  1. );
	std::vector<Eigen::Vector3f> scan{p3d0_gt};
	Particle p(Eigen::Vector3f(0, 0, 0), 1.0);
	std::vector<Particle> particles{p};
	std::vector<double> scanMask(1, 1.0);

	LidarData data = LidarData(scan, scanMask);

	be.ComputeWeights(particles, std::make_shared<LidarData>(data));

	//ASSERT_EQ(weights.size(), 1);

	// from python verified code weight should be 0.09063308, but the EDT is different therefore we expect some variance
	// Also sigma valued of BeamEnd was 8
	ASSERT_NEAR(particles[0].weight, 0.09063308, 0.01);
}



TEST(TestSetStatistics, test1)
{
	std::vector<Eigen::Vector3f> poses{Eigen::Vector3f(1,1,1), Eigen::Vector3f(1,1,1)};
	std::vector<double> weights {0.5, 0.5};
	Particle p1(Eigen::Vector3f(1,1,1), 0.5);
	Particle p2(Eigen::Vector3f(1,1,1), 0.5);
	std::vector<Particle> particles{p1, p2};


	SetStatistics stats = SetStatistics::ComputeParticleSetStatistics(particles);
	Eigen::Vector3d mean = stats.Mean();
	Eigen::Matrix3d cov = stats.Cov();

	ASSERT_EQ(mean, Eigen::Vector3d(1,1,1));

	ASSERT_NEAR(cov(0,0), 0, 0.000001);
	ASSERT_NEAR(cov(1,0), 0, 0.000001);
	ASSERT_NEAR(cov(0,1), 0, 0.000001);
	ASSERT_NEAR(cov(1,1), 0, 0.000001);
	ASSERT_NEAR(cov(2,2), 0, 0.000001);

}

TEST(TestSetStatistics, test2)
{
	std::vector<Eigen::Vector3f> poses{Eigen::Vector3f(1.3 ,1 ,1), Eigen::Vector3f(0.8, 0.7, 0)};
	std::vector<double> weights {0.5, 0.5};
	Particle p1(Eigen::Vector3f(1.3,1,1), 0.5);
	Particle p2(Eigen::Vector3f(0.8,0.7,0), 0.5);
	std::vector<Particle> particles{p1, p2};

	SetStatistics stats = SetStatistics::ComputeParticleSetStatistics(particles);
	Eigen::Vector3d mean = stats.Mean();
	Eigen::Matrix3d cov = stats.Cov();

	//ASSERT_EQ(mean, Eigen::Vector3d(1.05, 0.85, 0.5));
	ASSERT_NEAR(mean(0), 1.05 , 0.000001);
	ASSERT_NEAR(mean(1), 0.85 , 0.000001);
	ASSERT_NEAR(mean(2), 0.5 , 0.000001);
	ASSERT_NEAR(cov(0,0), 0.0625 , 0.000001);
	ASSERT_NEAR(cov(1,0), 0.0375 , 0.000001);
	ASSERT_NEAR(cov(0,1), 0.0375 , 0.000001);
	ASSERT_NEAR(cov(1,1), 0.0225, 0.000001);
	ASSERT_GE(cov(2,2), 0);  

}

TEST(TestSetStatistics, test3)
{
	std::vector<Eigen::Vector3f> poses{Eigen::Vector3f(1 ,1 ,1), Eigen::Vector3f(0, 0, 0)};
	std::vector<double> weights {1.0, 0.0};
	Particle p1(Eigen::Vector3f(1, 1,1 ), 1.0);
	Particle p2(Eigen::Vector3f(0, 0, 0), 0.0);
	std::vector<Particle> particles{p1, p2};

	SetStatistics stats = SetStatistics::ComputeParticleSetStatistics(particles);
	Eigen::Vector3d mean = stats.Mean();
	Eigen::Matrix3d cov = stats.Cov();

	ASSERT_EQ(mean, Eigen::Vector3d(1, 1, 1));

	ASSERT_NEAR(cov(0,0), 0, 0.000001);
	ASSERT_NEAR(cov(1,0), 0, 0.000001);
	ASSERT_NEAR(cov(0,1), 0, 0.000001);
	ASSERT_NEAR(cov(1,1), 0, 0.000001);
	ASSERT_NEAR(cov(2,2), 0, 0.000001);

}



TEST(TestNMCLFactory, test1)
{
	std::string configPath = testPath + "nmcltest.config";
	NMCLFactory::Dump(configPath);

	std::shared_ptr<ReNMCL> renmcl = NMCLFactory::Create(configPath);
	std::string roonName = renmcl->GetFloorMap()->GetRoomNames()[15];
	//std::cout << renmcl->GetFloorMap()->GetRoomNames()[1] << std::endl;


	ASSERT_EQ(roonName, "Room 1");
	//ASSERT(ups);

}

TEST(TestNMCLFactory, test2)
{
	std::string configPath = testPath + "nmcl.config";
	//NMCLFactory::Dump(configPath);

	std::shared_ptr<ReNMCL> renmcl = NMCLFactory::Create(configPath);
	std::string roonName = renmcl->GetFloorMap()->GetRoomNames()[15];
	//std::cout << renmcl->GetFloorMap()->GetRoomNames()[1] << std::endl;

	ASSERT_EQ(roonName, "Room 1");
}

TEST(TestNMCLFactory, test3)
{
	//std::string dataPath = PROJECT_TEST_DATA_DIR + std::string("/../../data/ABB/");
	std::string configPath =   testPath + "nmcl.config";
	std::shared_ptr<ReNMCL> renmcl = NMCLFactory::Create(configPath);

	std::vector<std::string> dict = renmcl->GetFloorMap()->GetRoomNames(); 
	PlaceRecognition placeRec = PlaceRecognition(dict, testPath + "TextMaps/");

	std::vector<std::string> places = {"Room 1"};
	std::vector<int> matches = placeRec.Match(places); 
	std::vector<std::string> confirmedMatches;
	TextData textData = placeRec.TextBoundingBoxes(matches, confirmedMatches);
	renmcl->Relocalize(textData.BottomRight(), textData.TopLeft(), textData.Orientation(), M_PI * 0.5);
	std::vector<Particle> particles = renmcl->Particles();

	std::ofstream particleFile;
    particleFile.open(dataPath + "particles.csv", std::ofstream::out);
    particleFile << "x" << "," << "y" << "," << "yaw" << "," << "w" << std::endl;
    for(int p = 0; p < particles.size(); ++p)
    {
        Eigen::Vector3f pose = particles[p].pose;
        float w = particles[p].weight;
        particleFile << pose(0) << "," << pose(1) << "," << pose(2) << "," << w << std::endl;
    }
    particleFile.close();
}






int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}