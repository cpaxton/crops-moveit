/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Author: Ioan Sucan */

#ifndef MOVEIT_ROBOT_INTERACTION_ROBOT_INTERACTION_
#define MOVEIT_ROBOT_INTERACTION_ROBOT_INTERACTION_

#include <visualization_msgs/InteractiveMarkerFeedback.h>
#include <visualization_msgs/InteractiveMarker.h>
#include <moveit/robot_state/robot_state.h>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <tf/tf.h>

namespace interactive_markers
{
class InteractiveMarkerServer;
}

namespace robot_interaction
{
  
class RobotInteraction
{
public:
  
  /// The topic name on which the internal Interactive Marker Server operates
  static const std::string INTERACTIVE_MARKER_TOPIC;
  
  struct EndEffector
  {
    /// The name of the group that sustains the end-effector (usually an arm)
    std::string parent_group;

    /// The name of the link in the parent group that connects to the end-effector
    std::string parent_link;

    /// The name of the group that defines the group joints
    std::string eef_group;
    
    /// The size of the end effector group (diameter of enclosing sphere)
    double size;
  };

  struct VirtualJoint
  { 
    /// The link in the robot model this joint connects to
    std::string connecting_link;
    
    /// The name of the virtual joint
    std::string joint_name;

    /// The number of DOF this virtual joint has
    unsigned int dof;

    /// The size of the connectig link  (diameter of enclosing sphere)
    double size;
  };
  
  class InteractionHandler;
  
  typedef boost::function<void(InteractionHandler*, bool)> InteractionHandlerCallbackFn;
  
  class InteractionHandler
  {
  public:

    InteractionHandler(const std::string &name,
                       const robot_state::RobotState &kstate,
                       const boost::shared_ptr<tf::Transformer> &tf = boost::shared_ptr<tf::Transformer>());
    InteractionHandler(const std::string &name,
                       const robot_model::RobotModelConstPtr &kmodel,
                       const boost::shared_ptr<tf::Transformer> &tf = boost::shared_ptr<tf::Transformer>());
    
    virtual ~InteractionHandler()
    {
    }
    
    const std::string& getName() const
    {
      return name_;
    }
    
    robot_state::RobotStateConstPtr getState() const;    
    void setState(const robot_state::RobotState& kstate);
    
    void setUpdateCallback(const InteractionHandlerCallbackFn &callback)
    {
      update_callback_ = callback;
    }    
    
    const InteractionHandlerCallbackFn& getUpdateCallback() const
    {
      return update_callback_;
    }    

    void setStateValidityCallback(const robot_state::StateValidityCallbackFn &callback)
    {
      state_validity_callback_fn_ = callback;
    }
    
    const robot_state::StateValidityCallbackFn& getStateValidityCallback() const
    {
      return state_validity_callback_fn_;
    }
    
    void setIKTimeout(double timeout)
    {
      ik_timeout_ = timeout;
    }
    
    double getIKTimeout() const
    {
      return ik_timeout_;
    }
    
    void setIKAttempts(unsigned int attempts)
    {
      ik_attempts_ = attempts;
    }
    
    unsigned int getIKAttempts() const
    {
      return ik_attempts_;
    }
        
    void setMeshesVisible(bool visible)
    {
      display_meshes_ = visible;
    }

    bool getMeshesVisible() const
    {
      return display_meshes_;
    }

    void setControlsVisible(bool visible)
    {
      display_controls_ = visible;
    }

    bool getControlsVisible() const
    {
      return display_controls_;
    }

    void setPoseOffset(const EndEffector& eef, const geometry_msgs::Pose& m);

    void clearPoseOffset(const RobotInteraction::EndEffector& eef);

    void clearPoseOffsets();

    bool getPoseOffset(const RobotInteraction::EndEffector& eef, geometry_msgs::Pose& m);
    bool getPoseOffset(const RobotInteraction::VirtualJoint& vj, geometry_msgs::Pose& m);
    
    /** \brief Get the last interactive_marker command pose for the end-effector
     * @param The end-effector in question.
     * @param A PoseStamped message containing the last (offset-adjusted) pose commanded for the end-effector.
     * @return True if a pose for that end-effector was found, false otherwise.
     */
    bool getLastEndEffectorMarkerPose(const RobotInteraction::EndEffector& eef, geometry_msgs::PoseStamped& pose);
    bool getLastVirtualJointMarkerPose(const RobotInteraction::VirtualJoint& vj, geometry_msgs::PoseStamped& pose);

    void clearLastEndEffectorMarkerPose(const RobotInteraction::EndEffector& eef);
    void clearLastVirtualJointMarkerPose(const RobotInteraction::VirtualJoint& vj);
    void clearLastMarkerPoses();

    /** \brief Update the internal state maintained by the handler using information from the received feedback message. Returns true if the marker should be updated (redrawn) and false otehrwise. */
    virtual bool handleEndEffector(const RobotInteraction::EndEffector &eef, const visualization_msgs::InteractiveMarkerFeedbackConstPtr &feedback);

    /** \brief Update the internal state maintained by the handler using information from the received feedback message. Returns true if the marker should be updated (redrawn) and false otehrwise. */
    virtual bool handleVirtualJoint(const RobotInteraction::VirtualJoint &vj, const visualization_msgs::InteractiveMarkerFeedbackConstPtr &feedback);

    virtual bool inError(const RobotInteraction::EndEffector& eef) const;
    virtual bool inError(const RobotInteraction::VirtualJoint& vj) const;
    
    void clearError(void);
    
  protected:

    bool transformFeedbackPose(const visualization_msgs::InteractiveMarkerFeedbackConstPtr &feedback,
                               const geometry_msgs::Pose &offset,
                               geometry_msgs::PoseStamped &tpose);

    robot_state::RobotStatePtr getUniqueStateAccess();
    void setStateToAccess(robot_state::RobotStatePtr &state);
    
    std::string name_;
    std::string planning_frame_;
    robot_state::RobotStatePtr kstate_;
    boost::shared_ptr<tf::Transformer> tf_;
    std::set<std::string> error_state_;
    std::map<std::string, geometry_msgs::PoseStamped> pose_map_;
    std::map<std::string, geometry_msgs::Pose> offset_map_;
    // bool can be used to signal a change in "state" (e.g. error, other properties?)
    boost::function<void(InteractionHandler*, bool)> update_callback_;
    robot_state::StateValidityCallbackFn state_validity_callback_fn_;
    double ik_timeout_;
    unsigned int ik_attempts_;
    bool display_meshes_;
    bool display_controls_;
    
  private:
    
    mutable boost::mutex state_lock_;
    mutable boost::condition_variable state_available_condition_;
    boost::mutex pose_map_lock_;
    boost::mutex offset_map_lock_;

    void setup();
  };

  typedef boost::shared_ptr<InteractionHandler> InteractionHandlerPtr;
  typedef boost::shared_ptr<const InteractionHandler> InteractionHandlerConstPtr;
  
  RobotInteraction(const robot_model::RobotModelConstPtr &kmodel, const std::string &ns = "");
  ~RobotInteraction();
  
  void decideActiveComponents(const std::string &group);
  void decideActiveEndEffectors(const std::string &group);
  void decideActiveVirtualJoints(const std::string &group);
  
  void clear();
  
  void addInteractiveMarkers(const InteractionHandlerPtr &handler, double marker_scale = 0.0);
  void updateInteractiveMarkers(const InteractionHandlerPtr &handler);
  bool showingMarkers(const InteractionHandlerPtr &handler);

  void publishInteractiveMarkers();
  void clearInteractiveMarkers();
  
  const std::vector<EndEffector>& getActiveEndEffectors() const
  {
    return active_eef_;
  }

  const std::vector<VirtualJoint>& getActiveVirtualJoints() const
  {
    return active_vj_;
  }
  
  static bool updateState(robot_state::RobotState &state, const EndEffector &eef, const geometry_msgs::Pose &pose,
                          unsigned int attempts, double ik_timeout, const robot_state::StateValidityCallbackFn &validity_callback = robot_state::StateValidityCallbackFn());
  static bool updateState(robot_state::RobotState &state, const VirtualJoint &vj, const geometry_msgs::Pose &pose);

private:
  
  // return the diameter of the sphere that certainly can enclose the AABB of the links in this group
  double computeGroupScale(const std::string &group);  
  void computeMarkerPose(const InteractionHandlerPtr &handler, const EndEffector &eef, const robot_state::RobotState &robot_state,
                         geometry_msgs::Pose &pose, geometry_msgs::Pose &control_to_eef_tf) const;
  
  void addEndEffectorMarkers(const InteractionHandlerPtr &handler, const EndEffector& eef, visualization_msgs::InteractiveMarker& im);
  void addEndEffectorMarkers(const InteractionHandlerPtr &handler, const EndEffector& eef, const geometry_msgs::Pose& offset, visualization_msgs::InteractiveMarker& im);
  void processInteractiveMarkerFeedback(const visualization_msgs::InteractiveMarkerFeedbackConstPtr& feedback);
  void processingThread();
  void clearInteractiveMarkersUnsafe();
  
  boost::scoped_ptr<boost::thread> processing_thread_;
  bool run_processing_thread_;

  boost::condition_variable new_feedback_condition_;
  std::map<std::string, visualization_msgs::InteractiveMarkerFeedbackConstPtr> feedback_map_;

  robot_model::RobotModelConstPtr robot_model_;
  
  std::vector<EndEffector> active_eef_;
  std::vector<VirtualJoint> active_vj_;
  
  std::map<std::string, InteractionHandlerPtr> handlers_;
  std::map<std::string, std::size_t> shown_markers_;
  boost::mutex marker_access_lock_;

  interactive_markers::InteractiveMarkerServer *int_marker_server_;
};

typedef boost::shared_ptr<RobotInteraction> RobotInteractionPtr;
typedef boost::shared_ptr<const RobotInteraction> RobotInteractionConstPtr;

}

#endif
