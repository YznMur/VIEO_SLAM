/**
 * This file is part of ORB-SLAM2.
 *
 * Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
 * For more information see <https://github.com/leavesnight/VIEO_SLAM>
 *
 * ORB-SLAM2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ORB-SLAM2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAPPOINT_H
#define MAPPOINT_H

#include "Map.h"

#include <opencv2/core/core.hpp>
#include <mutex>
#include <set>
#include <map>
#include <list>

namespace VIEO_SLAM {

class KeyFrame;
class Map;
class Frame;

class MapPoint {
 protected:
  template <typename _Tp>
  using list = std::list<_Tp>;
  using FrameId = unsigned long;
  typedef struct _TrackFastMatchInfo {
    // avg depth, for fast motion close scene
    float track_depth_ = INFINITY;
    // true for those in curf Frustum but last Frame untracked local map points
    bool btrack_inview_;
    static constexpr int NUM_PROJ = 3;
    list<float> vtrack_proj_[NUM_PROJ];  // X/Y/XR
    list<size_t> vtrack_cami_;
    list<float> vtrack_viewcos_;   // for r judge at lv0
    list<int> vtrack_scalelevel_;  // for r expansion lv judge and fast match search
    // for UpdateLocalPoints
    FrameId track_ref_frameid_;  // for plocalmappoints_
    FrameId last_seen_frameid_;  // for mp.IncreaseVisible()

    void Reset(Frame* pf = nullptr);
  } TrackFastMatchInfo;
  TrackFastMatchInfo trackinfo_ = {.track_ref_frameid_ = 0, .last_seen_frameid_ = 0};

 public:
  void UpdateScale(const float& scale);

  // for Load/SaveMap()
  MapPoint(KeyFrame* pRefKF, Map* pMap, std::istream& is);
  bool read(std::istream& is);
  bool write(std::ostream& os);

  // added by zzh

 public:
  MapPoint(const cv::Mat& Pos, KeyFrame* pRefKF, Map* pMap);
  MapPoint(const cv::Mat& Pos, Map* pMap, Frame* pFrame, const int& idxF);  // used in localization mode tracking

  cv::Mat GetWorldPos();
  void SetWorldPos(const cv::Mat& Pos, bool block = true);  // Pos.copyTo(mWorldPos)

  // if map is large, for single search time cost stability, please use map instead of unordered_map
  std::map<KeyFrame*, std::set<size_t>> GetObservations();  // mObservations
  void AddObservation(KeyFrame* pKF, size_t idx);           // mObservations[pKF]=idx;nObs+=2/1;
  // mObservations.erase(pKF), update nObs and when nObs<=2 ->SetBadFlag()
  void EraseObservation(KeyFrame* pKF, size_t idx = -1);
  std::set<size_t> GetIndexInKeyFrame(KeyFrame* pKF);                   // mObservations[pKF](empty() unfound)
  int Observations();                                                   // nObs
  bool IsInKeyFrame(KeyFrame* pKF, size_t idx = -1, size_t cami = -1);  // mObservations.count(pKF)

  bool isBad();       // mbBad
  void SetBadFlag();  // mbBad=true && delete this MP/matches in this->mObservations.first(KFs) && mObservations.clear()
                      // && delete this MP in mpMap

  KeyFrame* GetReferenceKeyFrame();  // mpRefKF, mpRefKF->mvpMapPoints should has this MP
  float GetMinDistanceInvariance();  // 0.8*mfMinDistance
  float GetMaxDistanceInvariance();  // 1.2*mfMaxDistance
  void UpdateNormalAndDepth();
  cv::Mat GetNormal();                   // mNormalVector
  void ComputeDistinctiveDescriptors();  // Take the descriptor with least median distance to the rest
  cv::Mat GetDescriptor();
  int PredictScale(const float& currentDist, KeyFrame* pKF);
  int PredictScale(const float& currentDist, Frame* pF);

  void IncreaseVisible(int n = 1);
  void IncreaseFound(int n = 1);
  float GetFoundRatio();

  void Replace(MapPoint* pMP);  // SetBadFlag() && replace this MP by pMP in this->mObservations.first(KFs)
  MapPoint* GetReplaced();

 public:
  long unsigned int mnId;
  static long unsigned int nNextId;
  long int mnFirstKFid;

  // Variables used by the tracking
  TrackFastMatchInfo& GetTrackInfoRef() { return trackinfo_; }

  // Variables used by local mapping
  long unsigned int mnBALocalForKF;        // local BA in LocalMapping
  long unsigned int mnFuseCandidateForKF;  // fuse in LocalMapping

  // Variables used by loop closing
  long unsigned int mnLoopPointForKF;      // used in ComputeSim3() in LoopClosing
  long unsigned int mnCorrectedByKF;       // avoid duplications, used in CorrectLoop()(&& PoseGraphOpt.) in LoopClosing
  long unsigned int mnCorrectedReference;  // ID of the KF which has corrected this MP's mWordPos, used in CorrectLoop()
                                           // && PoseGraphOpt. in Optimizer.cc
  cv::Mat mPosGBA;                         // optimized Pos in the end of GBA
  long unsigned int mnBAGlobalForKF;       // mpCurrentKF calling GBA

  static std::mutex mGlobalMutex;

 protected:
  // Position in absolute coordinates
  cv::Mat mWorldPos;

  // Keyframes observing the point and associated index in keyframe, set instead of vector here for simple coding
  std::map<KeyFrame*, std::set<size_t>> mObservations;

  int nObs;

  // Bad flag (we do not currently erase MapPoint from memory)
  bool mbBad;
  Map* mpMap;
  MapPoint* mpReplaced;

  // Mean viewing direction (not definitely normalized)
  cv::Mat mNormalVector;

  // Reference KeyFrame (the first KF in mObservations)
  KeyFrame* mpRefKF;
  // Scale invariance distances
  float mfMinDistance;
  float mfMaxDistance;

  // Best descriptor to fast matching
  cv::Mat mDescriptor;

  // Tracking counters
  int mnVisible;  // number of Frames with visible matches in SearchLocalPoints() in Tracking
  int mnFound;    // number of inliers in mnVisible in TrackLocalMap()/Track(for localization mode) in Tracking

  std::mutex mMutexPos;
  std::mutex mMutexFeatures;
};

}  // namespace VIEO_SLAM

#endif  // MAPPOINT_H
