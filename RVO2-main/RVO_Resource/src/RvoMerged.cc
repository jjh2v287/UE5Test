/*
 * Agent.cc
 * RVO2 Library
 *
 * SPDX-FileCopyrightText: 2008 University of North Carolina at Chapel Hill
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please send all bug reports to <geom@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Jur van den Berg, Stephen J. Guy, Jamie Snape, Ming C. Lin, Dinesh Manocha
 * Dept. of Computer Science
 * 201 S. Columbia St.
 * Frederick P. Brooks, Jr. Computer Science Bldg.
 * Chapel Hill, N.C. 27599-3175
 * United States of America
 *
 * <https://gamma.cs.unc.edu/RVO2/>
 */

/**
 * @file  Agent.cc
 * @brief Defines the Agent class.
 */

#include "Agent.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "KdTree.h"
#include "Obstacle.h"

namespace RVO {
namespace {
/**
 * @relates        Agent
 * @brief          Solves a one-dimensional linear program on a specified line
 *                 subject to linear constraints defined by lines and a circular
 *                 constraint.
 * @param[in]      lines        Lines defining the linear constraints.
 * @param[in]      lineNo       The specified line constraint.
 * @param[in]      radius       The radius of the circular constraint.
 * @param[in]      optVelocity  The optimization velocity.
 * @param[in]      directionOpt True if the direction should be optimized.
 * @param[in, out] result       A reference to the result of the linear program.
 * @return         True if successful.
 */
bool linearProgram1(const std::vector<Line> &lines, std::size_t lineNo,
                    float radius, const Vector2 &optVelocity, bool directionOpt,
                    Vector2 &result) { /* NOLINT(runtime/references) */
  const float dotProduct = lines[lineNo].point * lines[lineNo].direction;
  const float discriminant =
      dotProduct * dotProduct + radius * radius - absSq(lines[lineNo].point);

  if (discriminant < 0.0F) {
    /* Max speed circle fully invalidates line lineNo. */
    return false;
  }

  const float sqrtDiscriminant = std::sqrt(discriminant);
  float tLeft = -dotProduct - sqrtDiscriminant;
  float tRight = -dotProduct + sqrtDiscriminant;

  for (std::size_t i = 0U; i < lineNo; ++i) {
    const float denominator = det(lines[lineNo].direction, lines[i].direction);
    const float numerator =
        det(lines[i].direction, lines[lineNo].point - lines[i].point);

    if (std::fabs(denominator) <= RVO_EPSILON) {
      /* Lines lineNo and i are (almost) parallel. */
      if (numerator < 0.0F) {
        return false;
      }

      continue;
    }

    const float t = numerator / denominator;

    if (denominator >= 0.0F) {
      /* Line i bounds line lineNo on the right. */
      tRight = std::min(tRight, t);
    } else {
      /* Line i bounds line lineNo on the left. */
      tLeft = std::max(tLeft, t);
    }

    if (tLeft > tRight) {
      return false;
    }
  }

  if (directionOpt) {
    /* Optimize direction. */
    if (optVelocity * lines[lineNo].direction > 0.0F) {
      /* Take right extreme. */
      result = lines[lineNo].point + tRight * lines[lineNo].direction;
    } else {
      /* Take left extreme. */
      result = lines[lineNo].point + tLeft * lines[lineNo].direction;
    }
  } else {
    /* Optimize closest point. */
    const float t =
        lines[lineNo].direction * (optVelocity - lines[lineNo].point);

    if (t < tLeft) {
      result = lines[lineNo].point + tLeft * lines[lineNo].direction;
    } else if (t > tRight) {
      result = lines[lineNo].point + tRight * lines[lineNo].direction;
    } else {
      result = lines[lineNo].point + t * lines[lineNo].direction;
    }
  }

  return true;
}

/**
 * @relates        Agent
 * @brief          Solves a two-dimensional linear program subject to linear
 *                 constraints defined by lines and a circular constraint.
 * @param[in]      lines        Lines defining the linear constraints.
 * @param[in]      radius       The radius of the circular constraint.
 * @param[in]      optVelocity  The optimization velocity.
 * @param[in]      directionOpt True if the direction should be optimized.
 * @param[in, out] result       A reference to the result of the linear program.
 * @return         The number of the line it fails on, and the number of lines
 *                 if successful.
 */
std::size_t linearProgram2(const std::vector<Line> &lines, float radius,
                           const Vector2 &optVelocity, bool directionOpt,
                           Vector2 &result) { /* NOLINT(runtime/references) */
  if (directionOpt) {
    /* Optimize direction. Note that the optimization velocity is of unit length
     * in this case.
     */
    result = optVelocity * radius;
  } else if (absSq(optVelocity) > radius * radius) {
    /* Optimize closest point and outside circle. */
    result = normalize(optVelocity) * radius;
  } else {
    /* Optimize closest point and inside circle. */
    result = optVelocity;
  }

  for (std::size_t i = 0U; i < lines.size(); ++i) {
    if (det(lines[i].direction, lines[i].point - result) > 0.0F) {
      /* Result does not satisfy constraint i. Compute new optimal result. */
      const Vector2 tempResult = result;

      if (!linearProgram1(lines, i, radius, optVelocity, directionOpt,
                          result)) {
        result = tempResult;

        return i;
      }
    }
  }

  return lines.size();
}

/**
 * @relates        Agent
 * @brief          Solves a two-dimensional linear program subject to linear
 *                 constraints defined by lines and a circular constraint.
 * @param[in]      lines        Lines defining the linear constraints.
 * @param[in]      numObstLines Count of obstacle lines.
 * @param[in]      beginLine    The line on which the 2-d linear program failed.
 * @param[in]      radius       The radius of the circular constraint.
 * @param[in, out] result       A reference to the result of the linear program.
 */
void linearProgram3(const std::vector<Line> &lines, std::size_t numObstLines,
                    std::size_t beginLine, float radius,
                    Vector2 &result) { /* NOLINT(runtime/references) */
  float distance = 0.0F;

  for (std::size_t i = beginLine; i < lines.size(); ++i) {
    if (det(lines[i].direction, lines[i].point - result) > distance) {
      /* Result does not satisfy constraint of line i. */
      std::vector<Line> projLines(
          lines.begin(),
          lines.begin() + static_cast<std::ptrdiff_t>(numObstLines));

      for (std::size_t j = numObstLines; j < i; ++j) {
        Line line;

        const float determinant = det(lines[i].direction, lines[j].direction);

        if (std::fabs(determinant) <= RVO_EPSILON) {
          /* Line i and line j are parallel. */
          if (lines[i].direction * lines[j].direction > 0.0F) {
            /* Line i and line j point in the same direction. */
            continue;
          }

          /* Line i and line j point in opposite direction. */
          line.point = 0.5F * (lines[i].point + lines[j].point);
        } else {
          line.point = lines[i].point + (det(lines[j].direction,
                                             lines[i].point - lines[j].point) /
                                         determinant) *
                                            lines[i].direction;
        }

        line.direction = normalize(lines[j].direction - lines[i].direction);
        projLines.push_back(line);
      }

      const Vector2 tempResult = result;

      if (linearProgram2(
              projLines, radius,
              Vector2(-lines[i].direction.y(), lines[i].direction.x()), true,
              result) < projLines.size()) {
        /* This should in principle not happen. The result is by definition
         * already in the feasible region of this linear program. If it fails,
         * it is due to small floating point error, and the current result is
         * kept. */
        result = tempResult;
      }

      distance = det(lines[i].direction, lines[i].point - result);
    }
  }
}
} /* namespace */

Agent::Agent()
    : id_(0U),
      maxNeighbors_(0U),
      maxSpeed_(0.0F),
      neighborDist_(0.0F),
      radius_(0.0F),
      timeHorizon_(0.0F),
      timeHorizonObst_(0.0F) {}

Agent::~Agent() {}

void Agent::computeNeighbors(const KdTree *kdTree) {
  obstacleNeighbors_.clear();
  const float range = timeHorizonObst_ * maxSpeed_ + radius_;
  kdTree->computeObstacleNeighbors(this, range * range);

  agentNeighbors_.clear();

  if (maxNeighbors_ > 0U) {
    float rangeSq = neighborDist_ * neighborDist_;
    kdTree->computeAgentNeighbors(this, rangeSq);
  }
}

/* Search for the best new velocity. */
void Agent::computeNewVelocity(float timeStep) {
  orcaLines_.clear();

  const float invTimeHorizonObst = 1.0F / timeHorizonObst_;

  /* Create obstacle ORCA lines. */
  for (std::size_t i = 0U; i < obstacleNeighbors_.size(); ++i) {
    const Obstacle *obstacle1 = obstacleNeighbors_[i].second;
    const Obstacle *obstacle2 = obstacle1->next_;

    const Vector2 relativePosition1 = obstacle1->point_ - position_;
    const Vector2 relativePosition2 = obstacle2->point_ - position_;

    /* Check if velocity obstacle of obstacle is already taken care of by
     * previously constructed obstacle ORCA lines. */
    bool alreadyCovered = false;

    for (std::size_t j = 0U; j < orcaLines_.size(); ++j) {
      if (det(invTimeHorizonObst * relativePosition1 - orcaLines_[j].point,
              orcaLines_[j].direction) -
                  invTimeHorizonObst * radius_ >=
              -RVO_EPSILON &&
          det(invTimeHorizonObst * relativePosition2 - orcaLines_[j].point,
              orcaLines_[j].direction) -
                  invTimeHorizonObst * radius_ >=
              -RVO_EPSILON) {
        alreadyCovered = true;
        break;
      }
    }

    if (alreadyCovered) {
      continue;
    }

    /* Not yet covered. Check for collisions. */
    const float distSq1 = absSq(relativePosition1);
    const float distSq2 = absSq(relativePosition2);

    const float radiusSq = radius_ * radius_;

    const Vector2 obstacleVector = obstacle2->point_ - obstacle1->point_;
    const float s =
        (-relativePosition1 * obstacleVector) / absSq(obstacleVector);
    const float distSqLine = absSq(-relativePosition1 - s * obstacleVector);

    Line line;

    if (s < 0.0F && distSq1 <= radiusSq) {
      /* Collision with left vertex. Ignore if non-convex. */
      if (obstacle1->isConvex_) {
        line.point = Vector2(0.0F, 0.0F);
        line.direction =
            normalize(Vector2(-relativePosition1.y(), relativePosition1.x()));
        orcaLines_.push_back(line);
      }

      continue;
    }

    if (s > 1.0F && distSq2 <= radiusSq) {
      /* Collision with right vertex. Ignore if non-convex or if it will be
       * taken care of by neighoring obstace */
      if (obstacle2->isConvex_ &&
          det(relativePosition2, obstacle2->direction_) >= 0.0F) {
        line.point = Vector2(0.0F, 0.0F);
        line.direction =
            normalize(Vector2(-relativePosition2.y(), relativePosition2.x()));
        orcaLines_.push_back(line);
      }

      continue;
    }

    if (s >= 0.0F && s <= 1.0F && distSqLine <= radiusSq) {
      /* Collision with obstacle segment. */
      line.point = Vector2(0.0F, 0.0F);
      line.direction = -obstacle1->direction_;
      orcaLines_.push_back(line);
      continue;
    }

    /* No collision. Compute legs. When obliquely viewed, both legs can come
     * from a single vertex. Legs extend cut-off line when nonconvex vertex. */
    Vector2 leftLegDirection;
    Vector2 rightLegDirection;

    if (s < 0.0F && distSqLine <= radiusSq) {
      /* Obstacle viewed obliquely so that left vertex defines velocity
       * obstacle. */
      if (!obstacle1->isConvex_) {
        /* Ignore obstacle. */
        continue;
      }

      obstacle2 = obstacle1;

      const float leg1 = std::sqrt(distSq1 - radiusSq);
      leftLegDirection =
          Vector2(
              relativePosition1.x() * leg1 - relativePosition1.y() * radius_,
              relativePosition1.x() * radius_ + relativePosition1.y() * leg1) /
          distSq1;
      rightLegDirection =
          Vector2(
              relativePosition1.x() * leg1 + relativePosition1.y() * radius_,
              -relativePosition1.x() * radius_ + relativePosition1.y() * leg1) /
          distSq1;
    } else if (s > 1.0F && distSqLine <= radiusSq) {
      /* Obstacle viewed obliquely so that right vertex defines velocity
       * obstacle. */
      if (!obstacle2->isConvex_) {
        /* Ignore obstacle. */
        continue;
      }

      obstacle1 = obstacle2;

      const float leg2 = std::sqrt(distSq2 - radiusSq);
      leftLegDirection =
          Vector2(
              relativePosition2.x() * leg2 - relativePosition2.y() * radius_,
              relativePosition2.x() * radius_ + relativePosition2.y() * leg2) /
          distSq2;
      rightLegDirection =
          Vector2(
              relativePosition2.x() * leg2 + relativePosition2.y() * radius_,
              -relativePosition2.x() * radius_ + relativePosition2.y() * leg2) /
          distSq2;
    } else {
      /* Usual situation. */
      if (obstacle1->isConvex_) {
        const float leg1 = std::sqrt(distSq1 - radiusSq);
        leftLegDirection = Vector2(relativePosition1.x() * leg1 -
                                       relativePosition1.y() * radius_,
                                   relativePosition1.x() * radius_ +
                                       relativePosition1.y() * leg1) /
                           distSq1;
      } else {
        /* Left vertex non-convex; left leg extends cut-off line. */
        leftLegDirection = -obstacle1->direction_;
      }

      if (obstacle2->isConvex_) {
        const float leg2 = std::sqrt(distSq2 - radiusSq);
        rightLegDirection = Vector2(relativePosition2.x() * leg2 +
                                        relativePosition2.y() * radius_,
                                    -relativePosition2.x() * radius_ +
                                        relativePosition2.y() * leg2) /
                            distSq2;
      } else {
        /* Right vertex non-convex; right leg extends cut-off line. */
        rightLegDirection = obstacle1->direction_;
      }
    }

    /* Legs can never point into neighboring edge when convex vertex, take
     * cutoff-line of neighboring edge instead. If velocity projected on
     * "foreign" leg, no constraint is added. */
    const Obstacle *const leftNeighbor = obstacle1->previous_;

    bool isLeftLegForeign = false;
    bool isRightLegForeign = false;

    if (obstacle1->isConvex_ &&
        det(leftLegDirection, -leftNeighbor->direction_) >= 0.0F) {
      /* Left leg points into obstacle. */
      leftLegDirection = -leftNeighbor->direction_;
      isLeftLegForeign = true;
    }

    if (obstacle2->isConvex_ &&
        det(rightLegDirection, obstacle2->direction_) <= 0.0F) {
      /* Right leg points into obstacle. */
      rightLegDirection = obstacle2->direction_;
      isRightLegForeign = true;
    }

    /* Compute cut-off centers. */
    const Vector2 leftCutoff =
        invTimeHorizonObst * (obstacle1->point_ - position_);
    const Vector2 rightCutoff =
        invTimeHorizonObst * (obstacle2->point_ - position_);
    const Vector2 cutoffVector = rightCutoff - leftCutoff;

    /* Project current velocity on velocity obstacle. */

    /* Check if current velocity is projected on cutoff circles. */
    const float t =
        obstacle1 == obstacle2
            ? 0.5F
            : (velocity_ - leftCutoff) * cutoffVector / absSq(cutoffVector);
    const float tLeft = (velocity_ - leftCutoff) * leftLegDirection;
    const float tRight = (velocity_ - rightCutoff) * rightLegDirection;

    if ((t < 0.0F && tLeft < 0.0F) ||
        (obstacle1 == obstacle2 && tLeft < 0.0F && tRight < 0.0F)) {
      /* Project on left cut-off circle. */
      const Vector2 unitW = normalize(velocity_ - leftCutoff);

      line.direction = Vector2(unitW.y(), -unitW.x());
      line.point = leftCutoff + radius_ * invTimeHorizonObst * unitW;
      orcaLines_.push_back(line);
      continue;
    }

    if (t > 1.0F && tRight < 0.0F) {
      /* Project on right cut-off circle. */
      const Vector2 unitW = normalize(velocity_ - rightCutoff);

      line.direction = Vector2(unitW.y(), -unitW.x());
      line.point = rightCutoff + radius_ * invTimeHorizonObst * unitW;
      orcaLines_.push_back(line);
      continue;
    }

    /* Project on left leg, right leg, or cut-off line, whichever is closest to
     * velocity. */
    const float distSqCutoff =
        (t < 0.0F || t > 1.0F || obstacle1 == obstacle2)
            ? std::numeric_limits<float>::infinity()
            : absSq(velocity_ - (leftCutoff + t * cutoffVector));
    const float distSqLeft =
        tLeft < 0.0F
            ? std::numeric_limits<float>::infinity()
            : absSq(velocity_ - (leftCutoff + tLeft * leftLegDirection));
    const float distSqRight =
        tRight < 0.0F
            ? std::numeric_limits<float>::infinity()
            : absSq(velocity_ - (rightCutoff + tRight * rightLegDirection));

    if (distSqCutoff <= distSqLeft && distSqCutoff <= distSqRight) {
      /* Project on cut-off line. */
      line.direction = -obstacle1->direction_;
      line.point =
          leftCutoff + radius_ * invTimeHorizonObst *
                           Vector2(-line.direction.y(), line.direction.x());
      orcaLines_.push_back(line);
      continue;
    }

    if (distSqLeft <= distSqRight) {
      /* Project on left leg. */
      if (isLeftLegForeign) {
        continue;
      }

      line.direction = leftLegDirection;
      line.point =
          leftCutoff + radius_ * invTimeHorizonObst *
                           Vector2(-line.direction.y(), line.direction.x());
      orcaLines_.push_back(line);
      continue;
    }

    /* Project on right leg. */
    if (isRightLegForeign) {
      continue;
    }

    line.direction = -rightLegDirection;
    line.point =
        rightCutoff + radius_ * invTimeHorizonObst *
                          Vector2(-line.direction.y(), line.direction.x());
    orcaLines_.push_back(line);
  }

  const std::size_t numObstLines = orcaLines_.size();

  const float invTimeHorizon = 1.0F / timeHorizon_;

  /* Create agent ORCA lines. */
  for (std::size_t i = 0U; i < agentNeighbors_.size(); ++i) {
    const Agent *const other = agentNeighbors_[i].second;

    const Vector2 relativePosition = other->position_ - position_;
    const Vector2 relativeVelocity = velocity_ - other->velocity_;
    const float distSq = absSq(relativePosition);
    const float combinedRadius = radius_ + other->radius_;
    const float combinedRadiusSq = combinedRadius * combinedRadius;

    Line line;
    Vector2 u;

    if (distSq > combinedRadiusSq) {
      /* No collision. */
      const Vector2 w = relativeVelocity - invTimeHorizon * relativePosition;
      /* Vector from cutoff center to relative velocity. */
      const float wLengthSq = absSq(w);

      const float dotProduct = w * relativePosition;

      if (dotProduct < 0.0F &&
          dotProduct * dotProduct > combinedRadiusSq * wLengthSq) {
        /* Project on cut-off circle. */
        const float wLength = std::sqrt(wLengthSq);
        const Vector2 unitW = w / wLength;

        line.direction = Vector2(unitW.y(), -unitW.x());
        u = (combinedRadius * invTimeHorizon - wLength) * unitW;
      } else {
        /* Project on legs. */
        const float leg = std::sqrt(distSq - combinedRadiusSq);

        if (det(relativePosition, w) > 0.0F) {
          /* Project on left leg. */
          line.direction = Vector2(relativePosition.x() * leg -
                                       relativePosition.y() * combinedRadius,
                                   relativePosition.x() * combinedRadius +
                                       relativePosition.y() * leg) /
                           distSq;
        } else {
          /* Project on right leg. */
          line.direction = -Vector2(relativePosition.x() * leg +
                                        relativePosition.y() * combinedRadius,
                                    -relativePosition.x() * combinedRadius +
                                        relativePosition.y() * leg) /
                           distSq;
        }

        u = (relativeVelocity * line.direction) * line.direction -
            relativeVelocity;
      }
    } else {
      /* Collision. Project on cut-off circle of time timeStep. */
      const float invTimeStep = 1.0F / timeStep;

      /* Vector from cutoff center to relative velocity. */
      const Vector2 w = relativeVelocity - invTimeStep * relativePosition;

      const float wLength = abs(w);
      const Vector2 unitW = w / wLength;

      line.direction = Vector2(unitW.y(), -unitW.x());
      u = (combinedRadius * invTimeStep - wLength) * unitW;
    }

    line.point = velocity_ + 0.5F * u;
    orcaLines_.push_back(line);
  }

  const std::size_t lineFail =
      linearProgram2(orcaLines_, maxSpeed_, prefVelocity_, false, newVelocity_);

  if (lineFail < orcaLines_.size()) {
    linearProgram3(orcaLines_, numObstLines, lineFail, maxSpeed_, newVelocity_);
  }
}

void Agent::insertAgentNeighbor(const Agent *agent, float &rangeSq) {
  if (this != agent) {
    const float distSq = absSq(position_ - agent->position_);

    if (distSq < rangeSq) {
      if (agentNeighbors_.size() < maxNeighbors_) {
        agentNeighbors_.push_back(std::make_pair(distSq, agent));
      }

      std::size_t i = agentNeighbors_.size() - 1U;

      while (i != 0U && distSq < agentNeighbors_[i - 1U].first) {
        agentNeighbors_[i] = agentNeighbors_[i - 1U];
        --i;
      }

      agentNeighbors_[i] = std::make_pair(distSq, agent);

      if (agentNeighbors_.size() == maxNeighbors_) {
        rangeSq = agentNeighbors_.back().first;
      }
    }
  }
}

void Agent::insertObstacleNeighbor(const Obstacle *obstacle, float rangeSq) {
  const Obstacle *const nextObstacle = obstacle->next_;

  float distSq = 0.0F;
  const float r = ((position_ - obstacle->point_) *
                   (nextObstacle->point_ - obstacle->point_)) /
                  absSq(nextObstacle->point_ - obstacle->point_);

  if (r < 0.0F) {
    distSq = absSq(position_ - obstacle->point_);
  } else if (r > 1.0F) {
    distSq = absSq(position_ - nextObstacle->point_);
  } else {
    distSq = absSq(position_ - (obstacle->point_ +
                                r * (nextObstacle->point_ - obstacle->point_)));
  }

  if (distSq < rangeSq) {
    obstacleNeighbors_.push_back(std::make_pair(distSq, obstacle));

    std::size_t i = obstacleNeighbors_.size() - 1U;

    while (i != 0U && distSq < obstacleNeighbors_[i - 1U].first) {
      obstacleNeighbors_[i] = obstacleNeighbors_[i - 1U];
      --i;
    }

    obstacleNeighbors_[i] = std::make_pair(distSq, obstacle);
  }
}

void Agent::update(float timeStep) {
  velocity_ = newVelocity_;
  position_ += velocity_ * timeStep;
}
} /* namespace RVO */
/*
 * Export.cc
 * RVO2 Library
 *
 * SPDX-FileCopyrightText: 2008 University of North Carolina at Chapel Hill
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please send all bug reports to <geom@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Jur van den Berg, Stephen J. Guy, Jamie Snape, Ming C. Lin, Dinesh Manocha
 * Dept. of Computer Science
 * 201 S. Columbia St.
 * Frederick P. Brooks, Jr. Computer Science Bldg.
 * Chapel Hill, N.C. 27599-3175
 * United States of America
 *
 * <https://gamma.cs.unc.edu/RVO2/>
 */

#include "Export.h"
/*
 * KdTree.cc
 * RVO2 Library
 *
 * SPDX-FileCopyrightText: 2008 University of North Carolina at Chapel Hill
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please send all bug reports to <geom@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Jur van den Berg, Stephen J. Guy, Jamie Snape, Ming C. Lin, Dinesh Manocha
 * Dept. of Computer Science
 * 201 S. Columbia St.
 * Frederick P. Brooks, Jr. Computer Science Bldg.
 * Chapel Hill, N.C. 27599-3175
 * United States of America
 *
 * <https://gamma.cs.unc.edu/RVO2/>
 */

/**
 * @file  KdTree.cc
 * @brief Defines the KdTree class.
 */

#include "KdTree.h"

#include <algorithm>
#include <utility>

#include "Agent.h"
#include "Obstacle.h"
#include "RVOSimulator.h"
#include "Vector2.h"

namespace RVO {
namespace {
/**
 * @relates KdTree
 * @brief   The maximum k-D tree node leaf size.
 */
const std::size_t RVO_MAX_LEAF_SIZE = 10U;
} /* namespace */

/**
 * @brief Defines an agent k-D tree node.
 */
class KdTree::AgentTreeNode {
 public:
  /**
   * @brief Constructs an agent k-D tree node instance.
   */
  AgentTreeNode();

  /**
   * @brief The beginning node number.
   */
  std::size_t begin;

  /**
   * @brief The ending node number.
   */
  std::size_t end;

  /**
   * @brief The left node number.
   */
  std::size_t left;

  /**
   * @brief The right node number.
   */
  std::size_t right;

  /**
   * @brief The maximum x-coordinate.
   */
  float maxX;

  /**
   * @brief The maximum y-coordinate.
   */
  float maxY;

  /**
   * @brief The minimum x-coordinate.
   */
  float minX;

  /**
   * @brief The minimum y-coordinate.
   */
  float minY;
};

KdTree::AgentTreeNode::AgentTreeNode()
    : begin(0U),
      end(0U),
      left(0U),
      right(0U),
      maxX(0.0F),
      maxY(0.0F),
      minX(0.0F),
      minY(0.0F) {}

/**
 * @brief Defines an obstacle k-D tree node.
 */
class KdTree::ObstacleTreeNode {
 public:
  /**
   * @brief Constructs an obstacle k-D tree node instance.
   */
  ObstacleTreeNode();

  /**
   * @brief Destroys this obstacle k-D tree node instance.
   */
  ~ObstacleTreeNode();

  /**
   * @brief The obstacle number.
   */
  const Obstacle *obstacle;

  /**
   * @brief The left obstacle tree node.
   */
  ObstacleTreeNode *left;

  /**
   * @brief The right obstacle tree node.
   */
  ObstacleTreeNode *right;

 private:
  /* Not implemented. */
  ObstacleTreeNode(const ObstacleTreeNode &other);

  /* Not implemented. */
  ObstacleTreeNode &operator=(const ObstacleTreeNode &other);
};

KdTree::ObstacleTreeNode::ObstacleTreeNode()
    : obstacle(NULL), left(NULL), right(NULL) {}

KdTree::ObstacleTreeNode::~ObstacleTreeNode() {}

KdTree::KdTree(RVOSimulator *simulator)
    : obstacleTree_(NULL), simulator_(simulator) {}

KdTree::~KdTree() { deleteObstacleTree(obstacleTree_); }

void KdTree::buildAgentTree() {
  if (agents_.size() < simulator_->agents_.size()) {
    agents_.insert(agents_.end(),
                   simulator_->agents_.begin() +
                       static_cast<std::ptrdiff_t>(agents_.size()),
                   simulator_->agents_.end());
    agentTree_.resize(2U * agents_.size() - 1U);
  }

  if (!agents_.empty()) {
    buildAgentTreeRecursive(0U, agents_.size(), 0U);
  }
}

void KdTree::buildAgentTreeRecursive(std::size_t begin, std::size_t end,
                                     std::size_t node) {
  agentTree_[node].begin = begin;
  agentTree_[node].end = end;
  agentTree_[node].minX = agentTree_[node].maxX = agents_[begin]->position_.x();
  agentTree_[node].minY = agentTree_[node].maxY = agents_[begin]->position_.y();

  for (std::size_t i = begin + 1U; i < end; ++i) {
    agentTree_[node].maxX =
        std::max(agentTree_[node].maxX, agents_[i]->position_.x());
    agentTree_[node].minX =
        std::min(agentTree_[node].minX, agents_[i]->position_.x());
    agentTree_[node].maxY =
        std::max(agentTree_[node].maxY, agents_[i]->position_.y());
    agentTree_[node].minY =
        std::min(agentTree_[node].minY, agents_[i]->position_.y());
  }

  if (end - begin > RVO_MAX_LEAF_SIZE) {
    /* No leaf node. */
    const bool isVertical = agentTree_[node].maxX - agentTree_[node].minX >
                            agentTree_[node].maxY - agentTree_[node].minY;
    const float splitValue =
        0.5F * (isVertical ? agentTree_[node].maxX + agentTree_[node].minX
                           : agentTree_[node].maxY + agentTree_[node].minY);

    std::size_t left = begin;
    std::size_t right = end;

    while (left < right) {
      while (left < right &&
             (isVertical ? agents_[left]->position_.x()
                         : agents_[left]->position_.y()) < splitValue) {
        ++left;
      }

      while (right > left &&
             (isVertical ? agents_[right - 1U]->position_.x()
                         : agents_[right - 1U]->position_.y()) >= splitValue) {
        --right;
      }

      if (left < right) {
        std::swap(agents_[left], agents_[right - 1U]);
        ++left;
        --right;
      }
    }

    if (left == begin) {
      ++left;
      ++right;
    }

    agentTree_[node].left = node + 1U;
    agentTree_[node].right = node + 2U * (left - begin);

    buildAgentTreeRecursive(begin, left, agentTree_[node].left);
    buildAgentTreeRecursive(left, end, agentTree_[node].right);
  }
}

void KdTree::buildObstacleTree() {
  deleteObstacleTree(obstacleTree_);

  const std::vector<Obstacle *> obstacles(simulator_->obstacles_);
  obstacleTree_ = buildObstacleTreeRecursive(obstacles);
}

KdTree::ObstacleTreeNode *KdTree::buildObstacleTreeRecursive(
    const std::vector<Obstacle *> &obstacles) {
  if (!obstacles.empty()) {
    ObstacleTreeNode *const node = new ObstacleTreeNode();

    std::size_t optimalSplit = 0U;
    std::size_t minLeft = obstacles.size();
    std::size_t minRight = obstacles.size();

    for (std::size_t i = 0U; i < obstacles.size(); ++i) {
      std::size_t leftSize = 0U;
      std::size_t rightSize = 0U;

      const Obstacle *const obstacleI1 = obstacles[i];
      const Obstacle *const obstacleI2 = obstacleI1->next_;

      /* Compute optimal split node. */
      for (std::size_t j = 0U; j < obstacles.size(); ++j) {
        if (i != j) {
          const Obstacle *const obstacleJ1 = obstacles[j];
          const Obstacle *const obstacleJ2 = obstacleJ1->next_;

          const float j1LeftOfI = leftOf(obstacleI1->point_, obstacleI2->point_,
                                         obstacleJ1->point_);
          const float j2LeftOfI = leftOf(obstacleI1->point_, obstacleI2->point_,
                                         obstacleJ2->point_);

          if (j1LeftOfI >= -RVO_EPSILON && j2LeftOfI >= -RVO_EPSILON) {
            ++leftSize;
          } else if (j1LeftOfI <= RVO_EPSILON && j2LeftOfI <= RVO_EPSILON) {
            ++rightSize;
          } else {
            ++leftSize;
            ++rightSize;
          }

          if (std::make_pair(std::max(leftSize, rightSize),
                             std::min(leftSize, rightSize)) >=
              std::make_pair(std::max(minLeft, minRight),
                             std::min(minLeft, minRight))) {
            break;
          }
        }
      }

      if (std::make_pair(std::max(leftSize, rightSize),
                         std::min(leftSize, rightSize)) <
          std::make_pair(std::max(minLeft, minRight),
                         std::min(minLeft, minRight))) {
        minLeft = leftSize;
        minRight = rightSize;
        optimalSplit = i;
      }
    }

    /* Build split node. */
    std::vector<Obstacle *> leftObstacles(minLeft);
    std::vector<Obstacle *> rightObstacles(minRight);

    std::size_t leftCounter = 0U;
    std::size_t rightCounter = 0U;
    const std::size_t i = optimalSplit;

    const Obstacle *const obstacleI1 = obstacles[i];
    const Obstacle *const obstacleI2 = obstacleI1->next_;

    for (std::size_t j = 0U; j < obstacles.size(); ++j) {
      if (i != j) {
        Obstacle *const obstacleJ1 = obstacles[j];
        Obstacle *const obstacleJ2 = obstacleJ1->next_;

        const float j1LeftOfI =
            leftOf(obstacleI1->point_, obstacleI2->point_, obstacleJ1->point_);
        const float j2LeftOfI =
            leftOf(obstacleI1->point_, obstacleI2->point_, obstacleJ2->point_);

        if (j1LeftOfI >= -RVO_EPSILON && j2LeftOfI >= -RVO_EPSILON) {
          leftObstacles[leftCounter++] = obstacles[j];
        } else if (j1LeftOfI <= RVO_EPSILON && j2LeftOfI <= RVO_EPSILON) {
          rightObstacles[rightCounter++] = obstacles[j];
        } else {
          /* Split obstacle j. */
          const float t = det(obstacleI2->point_ - obstacleI1->point_,
                              obstacleJ1->point_ - obstacleI1->point_) /
                          det(obstacleI2->point_ - obstacleI1->point_,
                              obstacleJ1->point_ - obstacleJ2->point_);

          const Vector2 splitPoint =
              obstacleJ1->point_ +
              t * (obstacleJ2->point_ - obstacleJ1->point_);

          Obstacle *const newObstacle = new Obstacle();
          newObstacle->direction_ = obstacleJ1->direction_;
          newObstacle->point_ = splitPoint;
          newObstacle->next_ = obstacleJ2;
          newObstacle->previous_ = obstacleJ1;
          newObstacle->id_ = simulator_->obstacles_.size();
          newObstacle->isConvex_ = true;
          simulator_->obstacles_.push_back(newObstacle);

          obstacleJ1->next_ = newObstacle;
          obstacleJ2->previous_ = newObstacle;

          if (j1LeftOfI > 0.0F) {
            leftObstacles[leftCounter++] = obstacleJ1;
            rightObstacles[rightCounter++] = newObstacle;
          } else {
            rightObstacles[rightCounter++] = obstacleJ1;
            leftObstacles[leftCounter++] = newObstacle;
          }
        }
      }
    }

    node->obstacle = obstacleI1;
    node->left = buildObstacleTreeRecursive(leftObstacles);
    node->right = buildObstacleTreeRecursive(rightObstacles);

    return node;
  }

  return NULL;
}

void KdTree::computeAgentNeighbors(Agent *agent, float &rangeSq) const {
  queryAgentTreeRecursive(agent, rangeSq, 0U);
}

void KdTree::computeObstacleNeighbors(Agent *agent, float rangeSq) const {
  queryObstacleTreeRecursive(agent, rangeSq, obstacleTree_);
}

void KdTree::deleteObstacleTree(ObstacleTreeNode *node) {
  if (node != NULL) {
    deleteObstacleTree(node->left);
    deleteObstacleTree(node->right);
    delete node;
  }
}

void KdTree::queryAgentTreeRecursive(Agent *agent, float &rangeSq,
                                     std::size_t node) const {
  if (agentTree_[node].end - agentTree_[node].begin <= RVO_MAX_LEAF_SIZE) {
    for (std::size_t i = agentTree_[node].begin; i < agentTree_[node].end;
         ++i) {
      agent->insertAgentNeighbor(agents_[i], rangeSq);
    }
  } else {
    const float distLeftMinX = std::max(
        0.0F, agentTree_[agentTree_[node].left].minX - agent->position_.x());
    const float distLeftMaxX = std::max(
        0.0F, agent->position_.x() - agentTree_[agentTree_[node].left].maxX);
    const float distLeftMinY = std::max(
        0.0F, agentTree_[agentTree_[node].left].minY - agent->position_.y());
    const float distLeftMaxY = std::max(
        0.0F, agent->position_.y() - agentTree_[agentTree_[node].left].maxY);

    const float distSqLeft =
        distLeftMinX * distLeftMinX + distLeftMaxX * distLeftMaxX +
        distLeftMinY * distLeftMinY + distLeftMaxY * distLeftMaxY;

    const float distRightMinX = std::max(
        0.0F, agentTree_[agentTree_[node].right].minX - agent->position_.x());
    const float distRightMaxX = std::max(
        0.0F, agent->position_.x() - agentTree_[agentTree_[node].right].maxX);
    const float distRightMinY = std::max(
        0.0F, agentTree_[agentTree_[node].right].minY - agent->position_.y());
    const float distRightMaxY = std::max(
        0.0F, agent->position_.y() - agentTree_[agentTree_[node].right].maxY);

    const float distSqRight =
        distRightMinX * distRightMinX + distRightMaxX * distRightMaxX +
        distRightMinY * distRightMinY + distRightMaxY * distRightMaxY;

    if (distSqLeft < distSqRight) {
      if (distSqLeft < rangeSq) {
        queryAgentTreeRecursive(agent, rangeSq, agentTree_[node].left);

        if (distSqRight < rangeSq) {
          queryAgentTreeRecursive(agent, rangeSq, agentTree_[node].right);
        }
      }
    } else if (distSqRight < rangeSq) {
      queryAgentTreeRecursive(agent, rangeSq, agentTree_[node].right);

      if (distSqLeft < rangeSq) {
        queryAgentTreeRecursive(agent, rangeSq, agentTree_[node].left);
      }
    }
  }
}

void KdTree::queryObstacleTreeRecursive(Agent *agent, float rangeSq,
                                        const ObstacleTreeNode *node) const {
  if (node != NULL) {
    const Obstacle *const obstacle1 = node->obstacle;
    const Obstacle *const obstacle2 = obstacle1->next_;

    const float agentLeftOfLine =
        leftOf(obstacle1->point_, obstacle2->point_, agent->position_);

    queryObstacleTreeRecursive(
        agent, rangeSq, agentLeftOfLine >= 0.0F ? node->left : node->right);

    const float distSqLine = agentLeftOfLine * agentLeftOfLine /
                             absSq(obstacle2->point_ - obstacle1->point_);

    if (distSqLine < rangeSq) {
      if (agentLeftOfLine < 0.0F) {
        /* Try obstacle at this node only if agent is on right side of obstacle
         * and can see obstacle. */
        agent->insertObstacleNeighbor(node->obstacle, rangeSq);
      }

      /* Try other side of line. */
      queryObstacleTreeRecursive(
          agent, rangeSq, agentLeftOfLine >= 0.0F ? node->right : node->left);
    }
  }
}

bool KdTree::queryVisibility(const Vector2 &vector1, const Vector2 &vector2,
                             float radius) const {
  return queryVisibilityRecursive(vector1, vector2, radius, obstacleTree_);
}

bool KdTree::queryVisibilityRecursive(const Vector2 &vector1,
                                      const Vector2 &vector2, float radius,
                                      const ObstacleTreeNode *node) const {
  if (node != NULL) {
    const Obstacle *const obstacle1 = node->obstacle;
    const Obstacle *const obstacle2 = obstacle1->next_;

    const float q1LeftOfI =
        leftOf(obstacle1->point_, obstacle2->point_, vector1);
    const float q2LeftOfI =
        leftOf(obstacle1->point_, obstacle2->point_, vector2);
    const float invLengthI =
        1.0F / absSq(obstacle2->point_ - obstacle1->point_);

    if (q1LeftOfI >= 0.0F && q2LeftOfI >= 0.0F) {
      return queryVisibilityRecursive(vector1, vector2, radius, node->left) &&
             ((q1LeftOfI * q1LeftOfI * invLengthI >= radius * radius &&
               q2LeftOfI * q2LeftOfI * invLengthI >= radius * radius) ||
              queryVisibilityRecursive(vector1, vector2, radius, node->right));
    }

    if (q1LeftOfI <= 0.0F && q2LeftOfI <= 0.0F) {
      return queryVisibilityRecursive(vector1, vector2, radius, node->right) &&
             ((q1LeftOfI * q1LeftOfI * invLengthI >= radius * radius &&
               q2LeftOfI * q2LeftOfI * invLengthI >= radius * radius) ||
              queryVisibilityRecursive(vector1, vector2, radius, node->left));
    }

    if (q1LeftOfI >= 0.0F && q2LeftOfI <= 0.0F) {
      /* One can see through obstacle from left to right. */
      return queryVisibilityRecursive(vector1, vector2, radius, node->left) &&
             queryVisibilityRecursive(vector1, vector2, radius, node->right);
    }

    const float point1LeftOfQ = leftOf(vector1, vector2, obstacle1->point_);
    const float point2LeftOfQ = leftOf(vector1, vector2, obstacle2->point_);
    const float invLengthQ = 1.0F / absSq(vector2 - vector1);

    return point1LeftOfQ * point2LeftOfQ >= 0.0F &&
           point1LeftOfQ * point1LeftOfQ * invLengthQ > radius * radius &&
           point2LeftOfQ * point2LeftOfQ * invLengthQ > radius * radius &&
           queryVisibilityRecursive(vector1, vector2, radius, node->left) &&
           queryVisibilityRecursive(vector1, vector2, radius, node->right);
  }

  return true;
}
} /* namespace RVO */
/*
 * Line.cc
 * RVO2 Library
 *
 * SPDX-FileCopyrightText: 2008 University of North Carolina at Chapel Hill
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please send all bug reports to <geom@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Jur van den Berg, Stephen J. Guy, Jamie Snape, Ming C. Lin, Dinesh Manocha
 * Dept. of Computer Science
 * 201 S. Columbia St.
 * Frederick P. Brooks, Jr. Computer Science Bldg.
 * Chapel Hill, N.C. 27599-3175
 * United States of America
 *
 * <https://gamma.cs.unc.edu/RVO2/>
 */

/**
 * @file  Line.cc
 * @brief Defines the Line class.
 */

#include "Line.h"

namespace RVO {
Line::Line() {}
} /* namespace RVO */
/*
 * Obstacle.cc
 * RVO2 Library
 *
 * SPDX-FileCopyrightText: 2008 University of North Carolina at Chapel Hill
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please send all bug reports to <geom@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Jur van den Berg, Stephen J. Guy, Jamie Snape, Ming C. Lin, Dinesh Manocha
 * Dept. of Computer Science
 * 201 S. Columbia St.
 * Frederick P. Brooks, Jr. Computer Science Bldg.
 * Chapel Hill, N.C. 27599-3175
 * United States of America
 *
 * <https://gamma.cs.unc.edu/RVO2/>
 */

/**
 * @file  Obstacle.cc
 * @brief Defines the Obstacle class.
 */

#include "Obstacle.h"

namespace RVO {
Obstacle::Obstacle()
    : next_(NULL), previous_(NULL), id_(0U), isConvex_(false) {}

Obstacle::~Obstacle() {}
} /* namespace RVO */
/*
 * Agent.cc
 * RVO2 Library
 *
 * SPDX-FileCopyrightText: 2008 University of North Carolina at Chapel Hill
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please send all bug reports to <geom@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Jur van den Berg, Stephen J. Guy, Jamie Snape, Ming C. Lin, Dinesh Manocha
 * Dept. of Computer Science
 * 201 S. Columbia St.
 * Frederick P. Brooks, Jr. Computer Science Bldg.
 * Chapel Hill, N.C. 27599-3175
 * United States of America
 *
 * <https://gamma.cs.unc.edu/RVO2/>
 */

/**
 * @file  Agent.cc
 * @brief Defines the Agent class.
 */

#include "Agent.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "KdTree.h"
#include "Obstacle.h"

namespace RVO {
namespace {
/**
 * @relates        Agent
 * @brief          Solves a one-dimensional linear program on a specified line
 *                 subject to linear constraints defined by lines and a circular
 *                 constraint.
 * @param[in]      lines        Lines defining the linear constraints.
 * @param[in]      lineNo       The specified line constraint.
 * @param[in]      radius       The radius of the circular constraint.
 * @param[in]      optVelocity  The optimization velocity.
 * @param[in]      directionOpt True if the direction should be optimized.
 * @param[in, out] result       A reference to the result of the linear program.
 * @return         True if successful.
 */
bool linearProgram1(const std::vector<Line> &lines, std::size_t lineNo,
                    float radius, const Vector2 &optVelocity, bool directionOpt,
                    Vector2 &result) { /* NOLINT(runtime/references) */
  const float dotProduct = lines[lineNo].point * lines[lineNo].direction;
  const float discriminant =
      dotProduct * dotProduct + radius * radius - absSq(lines[lineNo].point);

  if (discriminant < 0.0F) {
    /* Max speed circle fully invalidates line lineNo. */
    return false;
  }

  const float sqrtDiscriminant = std::sqrt(discriminant);
  float tLeft = -dotProduct - sqrtDiscriminant;
  float tRight = -dotProduct + sqrtDiscriminant;

  for (std::size_t i = 0U; i < lineNo; ++i) {
    const float denominator = det(lines[lineNo].direction, lines[i].direction);
    const float numerator =
        det(lines[i].direction, lines[lineNo].point - lines[i].point);

    if (std::fabs(denominator) <= RVO_EPSILON) {
      /* Lines lineNo and i are (almost) parallel. */
      if (numerator < 0.0F) {
        return false;
      }

      continue;
    }

    const float t = numerator / denominator;

    if (denominator >= 0.0F) {
      /* Line i bounds line lineNo on the right. */
      tRight = std::min(tRight, t);
    } else {
      /* Line i bounds line lineNo on the left. */
      tLeft = std::max(tLeft, t);
    }

    if (tLeft > tRight) {
      return false;
    }
  }

  if (directionOpt) {
    /* Optimize direction. */
    if (optVelocity * lines[lineNo].direction > 0.0F) {
      /* Take right extreme. */
      result = lines[lineNo].point + tRight * lines[lineNo].direction;
    } else {
      /* Take left extreme. */
      result = lines[lineNo].point + tLeft * lines[lineNo].direction;
    }
  } else {
    /* Optimize closest point. */
    const float t =
        lines[lineNo].direction * (optVelocity - lines[lineNo].point);

    if (t < tLeft) {
      result = lines[lineNo].point + tLeft * lines[lineNo].direction;
    } else if (t > tRight) {
      result = lines[lineNo].point + tRight * lines[lineNo].direction;
    } else {
      result = lines[lineNo].point + t * lines[lineNo].direction;
    }
  }

  return true;
}

/**
 * @relates        Agent
 * @brief          Solves a two-dimensional linear program subject to linear
 *                 constraints defined by lines and a circular constraint.
 * @param[in]      lines        Lines defining the linear constraints.
 * @param[in]      radius       The radius of the circular constraint.
 * @param[in]      optVelocity  The optimization velocity.
 * @param[in]      directionOpt True if the direction should be optimized.
 * @param[in, out] result       A reference to the result of the linear program.
 * @return         The number of the line it fails on, and the number of lines
 *                 if successful.
 */
std::size_t linearProgram2(const std::vector<Line> &lines, float radius,
                           const Vector2 &optVelocity, bool directionOpt,
                           Vector2 &result) { /* NOLINT(runtime/references) */
  if (directionOpt) {
    /* Optimize direction. Note that the optimization velocity is of unit length
     * in this case.
     */
    result = optVelocity * radius;
  } else if (absSq(optVelocity) > radius * radius) {
    /* Optimize closest point and outside circle. */
    result = normalize(optVelocity) * radius;
  } else {
    /* Optimize closest point and inside circle. */
    result = optVelocity;
  }

  for (std::size_t i = 0U; i < lines.size(); ++i) {
    if (det(lines[i].direction, lines[i].point - result) > 0.0F) {
      /* Result does not satisfy constraint i. Compute new optimal result. */
      const Vector2 tempResult = result;

      if (!linearProgram1(lines, i, radius, optVelocity, directionOpt,
                          result)) {
        result = tempResult;

        return i;
      }
    }
  }

  return lines.size();
}

/**
 * @relates        Agent
 * @brief          Solves a two-dimensional linear program subject to linear
 *                 constraints defined by lines and a circular constraint.
 * @param[in]      lines        Lines defining the linear constraints.
 * @param[in]      numObstLines Count of obstacle lines.
 * @param[in]      beginLine    The line on which the 2-d linear program failed.
 * @param[in]      radius       The radius of the circular constraint.
 * @param[in, out] result       A reference to the result of the linear program.
 */
void linearProgram3(const std::vector<Line> &lines, std::size_t numObstLines,
                    std::size_t beginLine, float radius,
                    Vector2 &result) { /* NOLINT(runtime/references) */
  float distance = 0.0F;

  for (std::size_t i = beginLine; i < lines.size(); ++i) {
    if (det(lines[i].direction, lines[i].point - result) > distance) {
      /* Result does not satisfy constraint of line i. */
      std::vector<Line> projLines(
          lines.begin(),
          lines.begin() + static_cast<std::ptrdiff_t>(numObstLines));

      for (std::size_t j = numObstLines; j < i; ++j) {
        Line line;

        const float determinant = det(lines[i].direction, lines[j].direction);

        if (std::fabs(determinant) <= RVO_EPSILON) {
          /* Line i and line j are parallel. */
          if (lines[i].direction * lines[j].direction > 0.0F) {
            /* Line i and line j point in the same direction. */
            continue;
          }

          /* Line i and line j point in opposite direction. */
          line.point = 0.5F * (lines[i].point + lines[j].point);
        } else {
          line.point = lines[i].point + (det(lines[j].direction,
                                             lines[i].point - lines[j].point) /
                                         determinant) *
                                            lines[i].direction;
        }

        line.direction = normalize(lines[j].direction - lines[i].direction);
        projLines.push_back(line);
      }

      const Vector2 tempResult = result;

      if (linearProgram2(
              projLines, radius,
              Vector2(-lines[i].direction.y(), lines[i].direction.x()), true,
              result) < projLines.size()) {
        /* This should in principle not happen. The result is by definition
         * already in the feasible region of this linear program. If it fails,
         * it is due to small floating point error, and the current result is
         * kept. */
        result = tempResult;
      }

      distance = det(lines[i].direction, lines[i].point - result);
    }
  }
}
} /* namespace */

Agent::Agent()
    : id_(0U),
      maxNeighbors_(0U),
      maxSpeed_(0.0F),
      neighborDist_(0.0F),
      radius_(0.0F),
      timeHorizon_(0.0F),
      timeHorizonObst_(0.0F) {}

Agent::~Agent() {}

void Agent::computeNeighbors(const KdTree *kdTree) {
  obstacleNeighbors_.clear();
  const float range = timeHorizonObst_ * maxSpeed_ + radius_;
  kdTree->computeObstacleNeighbors(this, range * range);

  agentNeighbors_.clear();

  if (maxNeighbors_ > 0U) {
    float rangeSq = neighborDist_ * neighborDist_;
    kdTree->computeAgentNeighbors(this, rangeSq);
  }
}

/* Search for the best new velocity. */
void Agent::computeNewVelocity(float timeStep) {
  orcaLines_.clear();

  const float invTimeHorizonObst = 1.0F / timeHorizonObst_;

  /* Create obstacle ORCA lines. */
  for (std::size_t i = 0U; i < obstacleNeighbors_.size(); ++i) {
    const Obstacle *obstacle1 = obstacleNeighbors_[i].second;
    const Obstacle *obstacle2 = obstacle1->next_;

    const Vector2 relativePosition1 = obstacle1->point_ - position_;
    const Vector2 relativePosition2 = obstacle2->point_ - position_;

    /* Check if velocity obstacle of obstacle is already taken care of by
     * previously constructed obstacle ORCA lines. */
    bool alreadyCovered = false;

    for (std::size_t j = 0U; j < orcaLines_.size(); ++j) {
      if (det(invTimeHorizonObst * relativePosition1 - orcaLines_[j].point,
              orcaLines_[j].direction) -
                  invTimeHorizonObst * radius_ >=
              -RVO_EPSILON &&
          det(invTimeHorizonObst * relativePosition2 - orcaLines_[j].point,
              orcaLines_[j].direction) -
                  invTimeHorizonObst * radius_ >=
              -RVO_EPSILON) {
        alreadyCovered = true;
        break;
      }
    }

    if (alreadyCovered) {
      continue;
    }

    /* Not yet covered. Check for collisions. */
    const float distSq1 = absSq(relativePosition1);
    const float distSq2 = absSq(relativePosition2);

    const float radiusSq = radius_ * radius_;

    const Vector2 obstacleVector = obstacle2->point_ - obstacle1->point_;
    const float s =
        (-relativePosition1 * obstacleVector) / absSq(obstacleVector);
    const float distSqLine = absSq(-relativePosition1 - s * obstacleVector);

    Line line;

    if (s < 0.0F && distSq1 <= radiusSq) {
      /* Collision with left vertex. Ignore if non-convex. */
      if (obstacle1->isConvex_) {
        line.point = Vector2(0.0F, 0.0F);
        line.direction =
            normalize(Vector2(-relativePosition1.y(), relativePosition1.x()));
        orcaLines_.push_back(line);
      }

      continue;
    }

    if (s > 1.0F && distSq2 <= radiusSq) {
      /* Collision with right vertex. Ignore if non-convex or if it will be
       * taken care of by neighoring obstace */
      if (obstacle2->isConvex_ &&
          det(relativePosition2, obstacle2->direction_) >= 0.0F) {
        line.point = Vector2(0.0F, 0.0F);
        line.direction =
            normalize(Vector2(-relativePosition2.y(), relativePosition2.x()));
        orcaLines_.push_back(line);
      }

      continue;
    }

    if (s >= 0.0F && s <= 1.0F && distSqLine <= radiusSq) {
      /* Collision with obstacle segment. */
      line.point = Vector2(0.0F, 0.0F);
      line.direction = -obstacle1->direction_;
      orcaLines_.push_back(line);
      continue;
    }

    /* No collision. Compute legs. When obliquely viewed, both legs can come
     * from a single vertex. Legs extend cut-off line when nonconvex vertex. */
    Vector2 leftLegDirection;
    Vector2 rightLegDirection;

    if (s < 0.0F && distSqLine <= radiusSq) {
      /* Obstacle viewed obliquely so that left vertex defines velocity
       * obstacle. */
      if (!obstacle1->isConvex_) {
        /* Ignore obstacle. */
        continue;
      }

      obstacle2 = obstacle1;

      const float leg1 = std::sqrt(distSq1 - radiusSq);
      leftLegDirection =
          Vector2(
              relativePosition1.x() * leg1 - relativePosition1.y() * radius_,
              relativePosition1.x() * radius_ + relativePosition1.y() * leg1) /
          distSq1;
      rightLegDirection =
          Vector2(
              relativePosition1.x() * leg1 + relativePosition1.y() * radius_,
              -relativePosition1.x() * radius_ + relativePosition1.y() * leg1) /
          distSq1;
    } else if (s > 1.0F && distSqLine <= radiusSq) {
      /* Obstacle viewed obliquely so that right vertex defines velocity
       * obstacle. */
      if (!obstacle2->isConvex_) {
        /* Ignore obstacle. */
        continue;
      }

      obstacle1 = obstacle2;

      const float leg2 = std::sqrt(distSq2 - radiusSq);
      leftLegDirection =
          Vector2(
              relativePosition2.x() * leg2 - relativePosition2.y() * radius_,
              relativePosition2.x() * radius_ + relativePosition2.y() * leg2) /
          distSq2;
      rightLegDirection =
          Vector2(
              relativePosition2.x() * leg2 + relativePosition2.y() * radius_,
              -relativePosition2.x() * radius_ + relativePosition2.y() * leg2) /
          distSq2;
    } else {
      /* Usual situation. */
      if (obstacle1->isConvex_) {
        const float leg1 = std::sqrt(distSq1 - radiusSq);
        leftLegDirection = Vector2(relativePosition1.x() * leg1 -
                                       relativePosition1.y() * radius_,
                                   relativePosition1.x() * radius_ +
                                       relativePosition1.y() * leg1) /
                           distSq1;
      } else {
        /* Left vertex non-convex; left leg extends cut-off line. */
        leftLegDirection = -obstacle1->direction_;
      }

      if (obstacle2->isConvex_) {
        const float leg2 = std::sqrt(distSq2 - radiusSq);
        rightLegDirection = Vector2(relativePosition2.x() * leg2 +
                                        relativePosition2.y() * radius_,
                                    -relativePosition2.x() * radius_ +
                                        relativePosition2.y() * leg2) /
                            distSq2;
      } else {
        /* Right vertex non-convex; right leg extends cut-off line. */
        rightLegDirection = obstacle1->direction_;
      }
    }

    /* Legs can never point into neighboring edge when convex vertex, take
     * cutoff-line of neighboring edge instead. If velocity projected on
     * "foreign" leg, no constraint is added. */
    const Obstacle *const leftNeighbor = obstacle1->previous_;

    bool isLeftLegForeign = false;
    bool isRightLegForeign = false;

    if (obstacle1->isConvex_ &&
        det(leftLegDirection, -leftNeighbor->direction_) >= 0.0F) {
      /* Left leg points into obstacle. */
      leftLegDirection = -leftNeighbor->direction_;
      isLeftLegForeign = true;
    }

    if (obstacle2->isConvex_ &&
        det(rightLegDirection, obstacle2->direction_) <= 0.0F) {
      /* Right leg points into obstacle. */
      rightLegDirection = obstacle2->direction_;
      isRightLegForeign = true;
    }

    /* Compute cut-off centers. */
    const Vector2 leftCutoff =
        invTimeHorizonObst * (obstacle1->point_ - position_);
    const Vector2 rightCutoff =
        invTimeHorizonObst * (obstacle2->point_ - position_);
    const Vector2 cutoffVector = rightCutoff - leftCutoff;

    /* Project current velocity on velocity obstacle. */

    /* Check if current velocity is projected on cutoff circles. */
    const float t =
        obstacle1 == obstacle2
            ? 0.5F
            : (velocity_ - leftCutoff) * cutoffVector / absSq(cutoffVector);
    const float tLeft = (velocity_ - leftCutoff) * leftLegDirection;
    const float tRight = (velocity_ - rightCutoff) * rightLegDirection;

    if ((t < 0.0F && tLeft < 0.0F) ||
        (obstacle1 == obstacle2 && tLeft < 0.0F && tRight < 0.0F)) {
      /* Project on left cut-off circle. */
      const Vector2 unitW = normalize(velocity_ - leftCutoff);

      line.direction = Vector2(unitW.y(), -unitW.x());
      line.point = leftCutoff + radius_ * invTimeHorizonObst * unitW;
      orcaLines_.push_back(line);
      continue;
    }

    if (t > 1.0F && tRight < 0.0F) {
      /* Project on right cut-off circle. */
      const Vector2 unitW = normalize(velocity_ - rightCutoff);

      line.direction = Vector2(unitW.y(), -unitW.x());
      line.point = rightCutoff + radius_ * invTimeHorizonObst * unitW;
      orcaLines_.push_back(line);
      continue;
    }

    /* Project on left leg, right leg, or cut-off line, whichever is closest to
     * velocity. */
    const float distSqCutoff =
        (t < 0.0F || t > 1.0F || obstacle1 == obstacle2)
            ? std::numeric_limits<float>::infinity()
            : absSq(velocity_ - (leftCutoff + t * cutoffVector));
    const float distSqLeft =
        tLeft < 0.0F
            ? std::numeric_limits<float>::infinity()
            : absSq(velocity_ - (leftCutoff + tLeft * leftLegDirection));
    const float distSqRight =
        tRight < 0.0F
            ? std::numeric_limits<float>::infinity()
            : absSq(velocity_ - (rightCutoff + tRight * rightLegDirection));

    if (distSqCutoff <= distSqLeft && distSqCutoff <= distSqRight) {
      /* Project on cut-off line. */
      line.direction = -obstacle1->direction_;
      line.point =
          leftCutoff + radius_ * invTimeHorizonObst *
                           Vector2(-line.direction.y(), line.direction.x());
      orcaLines_.push_back(line);
      continue;
    }

    if (distSqLeft <= distSqRight) {
      /* Project on left leg. */
      if (isLeftLegForeign) {
        continue;
      }

      line.direction = leftLegDirection;
      line.point =
          leftCutoff + radius_ * invTimeHorizonObst *
                           Vector2(-line.direction.y(), line.direction.x());
      orcaLines_.push_back(line);
      continue;
    }

    /* Project on right leg. */
    if (isRightLegForeign) {
      continue;
    }

    line.direction = -rightLegDirection;
    line.point =
        rightCutoff + radius_ * invTimeHorizonObst *
                          Vector2(-line.direction.y(), line.direction.x());
    orcaLines_.push_back(line);
  }

  const std::size_t numObstLines = orcaLines_.size();

  const float invTimeHorizon = 1.0F / timeHorizon_;

  /* Create agent ORCA lines. */
  for (std::size_t i = 0U; i < agentNeighbors_.size(); ++i) {
    const Agent *const other = agentNeighbors_[i].second;

    const Vector2 relativePosition = other->position_ - position_;
    const Vector2 relativeVelocity = velocity_ - other->velocity_;
    const float distSq = absSq(relativePosition);
    const float combinedRadius = radius_ + other->radius_;
    const float combinedRadiusSq = combinedRadius * combinedRadius;

    Line line;
    Vector2 u;

    if (distSq > combinedRadiusSq) {
      /* No collision. */
      const Vector2 w = relativeVelocity - invTimeHorizon * relativePosition;
      /* Vector from cutoff center to relative velocity. */
      const float wLengthSq = absSq(w);

      const float dotProduct = w * relativePosition;

      if (dotProduct < 0.0F &&
          dotProduct * dotProduct > combinedRadiusSq * wLengthSq) {
        /* Project on cut-off circle. */
        const float wLength = std::sqrt(wLengthSq);
        const Vector2 unitW = w / wLength;

        line.direction = Vector2(unitW.y(), -unitW.x());
        u = (combinedRadius * invTimeHorizon - wLength) * unitW;
      } else {
        /* Project on legs. */
        const float leg = std::sqrt(distSq - combinedRadiusSq);

        if (det(relativePosition, w) > 0.0F) {
          /* Project on left leg. */
          line.direction = Vector2(relativePosition.x() * leg -
                                       relativePosition.y() * combinedRadius,
                                   relativePosition.x() * combinedRadius +
                                       relativePosition.y() * leg) /
                           distSq;
        } else {
          /* Project on right leg. */
          line.direction = -Vector2(relativePosition.x() * leg +
                                        relativePosition.y() * combinedRadius,
                                    -relativePosition.x() * combinedRadius +
                                        relativePosition.y() * leg) /
                           distSq;
        }

        u = (relativeVelocity * line.direction) * line.direction -
            relativeVelocity;
      }
    } else {
      /* Collision. Project on cut-off circle of time timeStep. */
      const float invTimeStep = 1.0F / timeStep;

      /* Vector from cutoff center to relative velocity. */
      const Vector2 w = relativeVelocity - invTimeStep * relativePosition;

      const float wLength = abs(w);
      const Vector2 unitW = w / wLength;

      line.direction = Vector2(unitW.y(), -unitW.x());
      u = (combinedRadius * invTimeStep - wLength) * unitW;
    }

    line.point = velocity_ + 0.5F * u;
    orcaLines_.push_back(line);
  }

  const std::size_t lineFail =
      linearProgram2(orcaLines_, maxSpeed_, prefVelocity_, false, newVelocity_);

  if (lineFail < orcaLines_.size()) {
    linearProgram3(orcaLines_, numObstLines, lineFail, maxSpeed_, newVelocity_);
  }
}

void Agent::insertAgentNeighbor(const Agent *agent, float &rangeSq) {
  if (this != agent) {
    const float distSq = absSq(position_ - agent->position_);

    if (distSq < rangeSq) {
      if (agentNeighbors_.size() < maxNeighbors_) {
        agentNeighbors_.push_back(std::make_pair(distSq, agent));
      }

      std::size_t i = agentNeighbors_.size() - 1U;

      while (i != 0U && distSq < agentNeighbors_[i - 1U].first) {
        agentNeighbors_[i] = agentNeighbors_[i - 1U];
        --i;
      }

      agentNeighbors_[i] = std::make_pair(distSq, agent);

      if (agentNeighbors_.size() == maxNeighbors_) {
        rangeSq = agentNeighbors_.back().first;
      }
    }
  }
}

void Agent::insertObstacleNeighbor(const Obstacle *obstacle, float rangeSq) {
  const Obstacle *const nextObstacle = obstacle->next_;

  float distSq = 0.0F;
  const float r = ((position_ - obstacle->point_) *
                   (nextObstacle->point_ - obstacle->point_)) /
                  absSq(nextObstacle->point_ - obstacle->point_);

  if (r < 0.0F) {
    distSq = absSq(position_ - obstacle->point_);
  } else if (r > 1.0F) {
    distSq = absSq(position_ - nextObstacle->point_);
  } else {
    distSq = absSq(position_ - (obstacle->point_ +
                                r * (nextObstacle->point_ - obstacle->point_)));
  }

  if (distSq < rangeSq) {
    obstacleNeighbors_.push_back(std::make_pair(distSq, obstacle));

    std::size_t i = obstacleNeighbors_.size() - 1U;

    while (i != 0U && distSq < obstacleNeighbors_[i - 1U].first) {
      obstacleNeighbors_[i] = obstacleNeighbors_[i - 1U];
      --i;
    }

    obstacleNeighbors_[i] = std::make_pair(distSq, obstacle);
  }
}

void Agent::update(float timeStep) {
  velocity_ = newVelocity_;
  position_ += velocity_ * timeStep;
}
} /* namespace RVO */
/*
 * Export.cc
 * RVO2 Library
 *
 * SPDX-FileCopyrightText: 2008 University of North Carolina at Chapel Hill
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please send all bug reports to <geom@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Jur van den Berg, Stephen J. Guy, Jamie Snape, Ming C. Lin, Dinesh Manocha
 * Dept. of Computer Science
 * 201 S. Columbia St.
 * Frederick P. Brooks, Jr. Computer Science Bldg.
 * Chapel Hill, N.C. 27599-3175
 * United States of America
 *
 * <https://gamma.cs.unc.edu/RVO2/>
 */

#include "Export.h"
/*
 * KdTree.cc
 * RVO2 Library
 *
 * SPDX-FileCopyrightText: 2008 University of North Carolina at Chapel Hill
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please send all bug reports to <geom@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Jur van den Berg, Stephen J. Guy, Jamie Snape, Ming C. Lin, Dinesh Manocha
 * Dept. of Computer Science
 * 201 S. Columbia St.
 * Frederick P. Brooks, Jr. Computer Science Bldg.
 * Chapel Hill, N.C. 27599-3175
 * United States of America
 *
 * <https://gamma.cs.unc.edu/RVO2/>
 */

/**
 * @file  KdTree.cc
 * @brief Defines the KdTree class.
 */

#include "KdTree.h"

#include <algorithm>
#include <utility>

#include "Agent.h"
#include "Obstacle.h"
#include "RVOSimulator.h"
#include "Vector2.h"

namespace RVO {
namespace {
/**
 * @relates KdTree
 * @brief   The maximum k-D tree node leaf size.
 */
const std::size_t RVO_MAX_LEAF_SIZE = 10U;
} /* namespace */

/**
 * @brief Defines an agent k-D tree node.
 */
class KdTree::AgentTreeNode {
 public:
  /**
   * @brief Constructs an agent k-D tree node instance.
   */
  AgentTreeNode();

  /**
   * @brief The beginning node number.
   */
  std::size_t begin;

  /**
   * @brief The ending node number.
   */
  std::size_t end;

  /**
   * @brief The left node number.
   */
  std::size_t left;

  /**
   * @brief The right node number.
   */
  std::size_t right;

  /**
   * @brief The maximum x-coordinate.
   */
  float maxX;

  /**
   * @brief The maximum y-coordinate.
   */
  float maxY;

  /**
   * @brief The minimum x-coordinate.
   */
  float minX;

  /**
   * @brief The minimum y-coordinate.
   */
  float minY;
};

KdTree::AgentTreeNode::AgentTreeNode()
    : begin(0U),
      end(0U),
      left(0U),
      right(0U),
      maxX(0.0F),
      maxY(0.0F),
      minX(0.0F),
      minY(0.0F) {}

/**
 * @brief Defines an obstacle k-D tree node.
 */
class KdTree::ObstacleTreeNode {
 public:
  /**
   * @brief Constructs an obstacle k-D tree node instance.
   */
  ObstacleTreeNode();

  /**
   * @brief Destroys this obstacle k-D tree node instance.
   */
  ~ObstacleTreeNode();

  /**
   * @brief The obstacle number.
   */
  const Obstacle *obstacle;

  /**
   * @brief The left obstacle tree node.
   */
  ObstacleTreeNode *left;

  /**
   * @brief The right obstacle tree node.
   */
  ObstacleTreeNode *right;

 private:
  /* Not implemented. */
  ObstacleTreeNode(const ObstacleTreeNode &other);

  /* Not implemented. */
  ObstacleTreeNode &operator=(const ObstacleTreeNode &other);
};

KdTree::ObstacleTreeNode::ObstacleTreeNode()
    : obstacle(NULL), left(NULL), right(NULL) {}

KdTree::ObstacleTreeNode::~ObstacleTreeNode() {}

KdTree::KdTree(RVOSimulator *simulator)
    : obstacleTree_(NULL), simulator_(simulator) {}

KdTree::~KdTree() { deleteObstacleTree(obstacleTree_); }

void KdTree::buildAgentTree() {
  if (agents_.size() < simulator_->agents_.size()) {
    agents_.insert(agents_.end(),
                   simulator_->agents_.begin() +
                       static_cast<std::ptrdiff_t>(agents_.size()),
                   simulator_->agents_.end());
    agentTree_.resize(2U * agents_.size() - 1U);
  }

  if (!agents_.empty()) {
    buildAgentTreeRecursive(0U, agents_.size(), 0U);
  }
}

void KdTree::buildAgentTreeRecursive(std::size_t begin, std::size_t end,
                                     std::size_t node) {
  agentTree_[node].begin = begin;
  agentTree_[node].end = end;
  agentTree_[node].minX = agentTree_[node].maxX = agents_[begin]->position_.x();
  agentTree_[node].minY = agentTree_[node].maxY = agents_[begin]->position_.y();

  for (std::size_t i = begin + 1U; i < end; ++i) {
    agentTree_[node].maxX =
        std::max(agentTree_[node].maxX, agents_[i]->position_.x());
    agentTree_[node].minX =
        std::min(agentTree_[node].minX, agents_[i]->position_.x());
    agentTree_[node].maxY =
        std::max(agentTree_[node].maxY, agents_[i]->position_.y());
    agentTree_[node].minY =
        std::min(agentTree_[node].minY, agents_[i]->position_.y());
  }

  if (end - begin > RVO_MAX_LEAF_SIZE) {
    /* No leaf node. */
    const bool isVertical = agentTree_[node].maxX - agentTree_[node].minX >
                            agentTree_[node].maxY - agentTree_[node].minY;
    const float splitValue =
        0.5F * (isVertical ? agentTree_[node].maxX + agentTree_[node].minX
                           : agentTree_[node].maxY + agentTree_[node].minY);

    std::size_t left = begin;
    std::size_t right = end;

    while (left < right) {
      while (left < right &&
             (isVertical ? agents_[left]->position_.x()
                         : agents_[left]->position_.y()) < splitValue) {
        ++left;
      }

      while (right > left &&
             (isVertical ? agents_[right - 1U]->position_.x()
                         : agents_[right - 1U]->position_.y()) >= splitValue) {
        --right;
      }

      if (left < right) {
        std::swap(agents_[left], agents_[right - 1U]);
        ++left;
        --right;
      }
    }

    if (left == begin) {
      ++left;
      ++right;
    }

    agentTree_[node].left = node + 1U;
    agentTree_[node].right = node + 2U * (left - begin);

    buildAgentTreeRecursive(begin, left, agentTree_[node].left);
    buildAgentTreeRecursive(left, end, agentTree_[node].right);
  }
}

void KdTree::buildObstacleTree() {
  deleteObstacleTree(obstacleTree_);

  const std::vector<Obstacle *> obstacles(simulator_->obstacles_);
  obstacleTree_ = buildObstacleTreeRecursive(obstacles);
}

KdTree::ObstacleTreeNode *KdTree::buildObstacleTreeRecursive(
    const std::vector<Obstacle *> &obstacles) {
  if (!obstacles.empty()) {
    ObstacleTreeNode *const node = new ObstacleTreeNode();

    std::size_t optimalSplit = 0U;
    std::size_t minLeft = obstacles.size();
    std::size_t minRight = obstacles.size();

    for (std::size_t i = 0U; i < obstacles.size(); ++i) {
      std::size_t leftSize = 0U;
      std::size_t rightSize = 0U;

      const Obstacle *const obstacleI1 = obstacles[i];
      const Obstacle *const obstacleI2 = obstacleI1->next_;

      /* Compute optimal split node. */
      for (std::size_t j = 0U; j < obstacles.size(); ++j) {
        if (i != j) {
          const Obstacle *const obstacleJ1 = obstacles[j];
          const Obstacle *const obstacleJ2 = obstacleJ1->next_;

          const float j1LeftOfI = leftOf(obstacleI1->point_, obstacleI2->point_,
                                         obstacleJ1->point_);
          const float j2LeftOfI = leftOf(obstacleI1->point_, obstacleI2->point_,
                                         obstacleJ2->point_);

          if (j1LeftOfI >= -RVO_EPSILON && j2LeftOfI >= -RVO_EPSILON) {
            ++leftSize;
          } else if (j1LeftOfI <= RVO_EPSILON && j2LeftOfI <= RVO_EPSILON) {
            ++rightSize;
          } else {
            ++leftSize;
            ++rightSize;
          }

          if (std::make_pair(std::max(leftSize, rightSize),
                             std::min(leftSize, rightSize)) >=
              std::make_pair(std::max(minLeft, minRight),
                             std::min(minLeft, minRight))) {
            break;
          }
        }
      }

      if (std::make_pair(std::max(leftSize, rightSize),
                         std::min(leftSize, rightSize)) <
          std::make_pair(std::max(minLeft, minRight),
                         std::min(minLeft, minRight))) {
        minLeft = leftSize;
        minRight = rightSize;
        optimalSplit = i;
      }
    }

    /* Build split node. */
    std::vector<Obstacle *> leftObstacles(minLeft);
    std::vector<Obstacle *> rightObstacles(minRight);

    std::size_t leftCounter = 0U;
    std::size_t rightCounter = 0U;
    const std::size_t i = optimalSplit;

    const Obstacle *const obstacleI1 = obstacles[i];
    const Obstacle *const obstacleI2 = obstacleI1->next_;

    for (std::size_t j = 0U; j < obstacles.size(); ++j) {
      if (i != j) {
        Obstacle *const obstacleJ1 = obstacles[j];
        Obstacle *const obstacleJ2 = obstacleJ1->next_;

        const float j1LeftOfI =
            leftOf(obstacleI1->point_, obstacleI2->point_, obstacleJ1->point_);
        const float j2LeftOfI =
            leftOf(obstacleI1->point_, obstacleI2->point_, obstacleJ2->point_);

        if (j1LeftOfI >= -RVO_EPSILON && j2LeftOfI >= -RVO_EPSILON) {
          leftObstacles[leftCounter++] = obstacles[j];
        } else if (j1LeftOfI <= RVO_EPSILON && j2LeftOfI <= RVO_EPSILON) {
          rightObstacles[rightCounter++] = obstacles[j];
        } else {
          /* Split obstacle j. */
          const float t = det(obstacleI2->point_ - obstacleI1->point_,
                              obstacleJ1->point_ - obstacleI1->point_) /
                          det(obstacleI2->point_ - obstacleI1->point_,
                              obstacleJ1->point_ - obstacleJ2->point_);

          const Vector2 splitPoint =
              obstacleJ1->point_ +
              t * (obstacleJ2->point_ - obstacleJ1->point_);

          Obstacle *const newObstacle = new Obstacle();
          newObstacle->direction_ = obstacleJ1->direction_;
          newObstacle->point_ = splitPoint;
          newObstacle->next_ = obstacleJ2;
          newObstacle->previous_ = obstacleJ1;
          newObstacle->id_ = simulator_->obstacles_.size();
          newObstacle->isConvex_ = true;
          simulator_->obstacles_.push_back(newObstacle);

          obstacleJ1->next_ = newObstacle;
          obstacleJ2->previous_ = newObstacle;

          if (j1LeftOfI > 0.0F) {
            leftObstacles[leftCounter++] = obstacleJ1;
            rightObstacles[rightCounter++] = newObstacle;
          } else {
            rightObstacles[rightCounter++] = obstacleJ1;
            leftObstacles[leftCounter++] = newObstacle;
          }
        }
      }
    }

    node->obstacle = obstacleI1;
    node->left = buildObstacleTreeRecursive(leftObstacles);
    node->right = buildObstacleTreeRecursive(rightObstacles);

    return node;
  }

  return NULL;
}

void KdTree::computeAgentNeighbors(Agent *agent, float &rangeSq) const {
  queryAgentTreeRecursive(agent, rangeSq, 0U);
}

void KdTree::computeObstacleNeighbors(Agent *agent, float rangeSq) const {
  queryObstacleTreeRecursive(agent, rangeSq, obstacleTree_);
}

void KdTree::deleteObstacleTree(ObstacleTreeNode *node) {
  if (node != NULL) {
    deleteObstacleTree(node->left);
    deleteObstacleTree(node->right);
    delete node;
  }
}

void KdTree::queryAgentTreeRecursive(Agent *agent, float &rangeSq,
                                     std::size_t node) const {
  if (agentTree_[node].end - agentTree_[node].begin <= RVO_MAX_LEAF_SIZE) {
    for (std::size_t i = agentTree_[node].begin; i < agentTree_[node].end;
         ++i) {
      agent->insertAgentNeighbor(agents_[i], rangeSq);
    }
  } else {
    const float distLeftMinX = std::max(
        0.0F, agentTree_[agentTree_[node].left].minX - agent->position_.x());
    const float distLeftMaxX = std::max(
        0.0F, agent->position_.x() - agentTree_[agentTree_[node].left].maxX);
    const float distLeftMinY = std::max(
        0.0F, agentTree_[agentTree_[node].left].minY - agent->position_.y());
    const float distLeftMaxY = std::max(
        0.0F, agent->position_.y() - agentTree_[agentTree_[node].left].maxY);

    const float distSqLeft =
        distLeftMinX * distLeftMinX + distLeftMaxX * distLeftMaxX +
        distLeftMinY * distLeftMinY + distLeftMaxY * distLeftMaxY;

    const float distRightMinX = std::max(
        0.0F, agentTree_[agentTree_[node].right].minX - agent->position_.x());
    const float distRightMaxX = std::max(
        0.0F, agent->position_.x() - agentTree_[agentTree_[node].right].maxX);
    const float distRightMinY = std::max(
        0.0F, agentTree_[agentTree_[node].right].minY - agent->position_.y());
    const float distRightMaxY = std::max(
        0.0F, agent->position_.y() - agentTree_[agentTree_[node].right].maxY);

    const float distSqRight =
        distRightMinX * distRightMinX + distRightMaxX * distRightMaxX +
        distRightMinY * distRightMinY + distRightMaxY * distRightMaxY;

    if (distSqLeft < distSqRight) {
      if (distSqLeft < rangeSq) {
        queryAgentTreeRecursive(agent, rangeSq, agentTree_[node].left);

        if (distSqRight < rangeSq) {
          queryAgentTreeRecursive(agent, rangeSq, agentTree_[node].right);
        }
      }
    } else if (distSqRight < rangeSq) {
      queryAgentTreeRecursive(agent, rangeSq, agentTree_[node].right);

      if (distSqLeft < rangeSq) {
        queryAgentTreeRecursive(agent, rangeSq, agentTree_[node].left);
      }
    }
  }
}

void KdTree::queryObstacleTreeRecursive(Agent *agent, float rangeSq,
                                        const ObstacleTreeNode *node) const {
  if (node != NULL) {
    const Obstacle *const obstacle1 = node->obstacle;
    const Obstacle *const obstacle2 = obstacle1->next_;

    const float agentLeftOfLine =
        leftOf(obstacle1->point_, obstacle2->point_, agent->position_);

    queryObstacleTreeRecursive(
        agent, rangeSq, agentLeftOfLine >= 0.0F ? node->left : node->right);

    const float distSqLine = agentLeftOfLine * agentLeftOfLine /
                             absSq(obstacle2->point_ - obstacle1->point_);

    if (distSqLine < rangeSq) {
      if (agentLeftOfLine < 0.0F) {
        /* Try obstacle at this node only if agent is on right side of obstacle
         * and can see obstacle. */
        agent->insertObstacleNeighbor(node->obstacle, rangeSq);
      }

      /* Try other side of line. */
      queryObstacleTreeRecursive(
          agent, rangeSq, agentLeftOfLine >= 0.0F ? node->right : node->left);
    }
  }
}

bool KdTree::queryVisibility(const Vector2 &vector1, const Vector2 &vector2,
                             float radius) const {
  return queryVisibilityRecursive(vector1, vector2, radius, obstacleTree_);
}

bool KdTree::queryVisibilityRecursive(const Vector2 &vector1,
                                      const Vector2 &vector2, float radius,
                                      const ObstacleTreeNode *node) const {
  if (node != NULL) {
    const Obstacle *const obstacle1 = node->obstacle;
    const Obstacle *const obstacle2 = obstacle1->next_;

    const float q1LeftOfI =
        leftOf(obstacle1->point_, obstacle2->point_, vector1);
    const float q2LeftOfI =
        leftOf(obstacle1->point_, obstacle2->point_, vector2);
    const float invLengthI =
        1.0F / absSq(obstacle2->point_ - obstacle1->point_);

    if (q1LeftOfI >= 0.0F && q2LeftOfI >= 0.0F) {
      return queryVisibilityRecursive(vector1, vector2, radius, node->left) &&
             ((q1LeftOfI * q1LeftOfI * invLengthI >= radius * radius &&
               q2LeftOfI * q2LeftOfI * invLengthI >= radius * radius) ||
              queryVisibilityRecursive(vector1, vector2, radius, node->right));
    }

    if (q1LeftOfI <= 0.0F && q2LeftOfI <= 0.0F) {
      return queryVisibilityRecursive(vector1, vector2, radius, node->right) &&
             ((q1LeftOfI * q1LeftOfI * invLengthI >= radius * radius &&
               q2LeftOfI * q2LeftOfI * invLengthI >= radius * radius) ||
              queryVisibilityRecursive(vector1, vector2, radius, node->left));
    }

    if (q1LeftOfI >= 0.0F && q2LeftOfI <= 0.0F) {
      /* One can see through obstacle from left to right. */
      return queryVisibilityRecursive(vector1, vector2, radius, node->left) &&
             queryVisibilityRecursive(vector1, vector2, radius, node->right);
    }

    const float point1LeftOfQ = leftOf(vector1, vector2, obstacle1->point_);
    const float point2LeftOfQ = leftOf(vector1, vector2, obstacle2->point_);
    const float invLengthQ = 1.0F / absSq(vector2 - vector1);

    return point1LeftOfQ * point2LeftOfQ >= 0.0F &&
           point1LeftOfQ * point1LeftOfQ * invLengthQ > radius * radius &&
           point2LeftOfQ * point2LeftOfQ * invLengthQ > radius * radius &&
           queryVisibilityRecursive(vector1, vector2, radius, node->left) &&
           queryVisibilityRecursive(vector1, vector2, radius, node->right);
  }

  return true;
}
} /* namespace RVO */
/*
 * Line.cc
 * RVO2 Library
 *
 * SPDX-FileCopyrightText: 2008 University of North Carolina at Chapel Hill
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please send all bug reports to <geom@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Jur van den Berg, Stephen J. Guy, Jamie Snape, Ming C. Lin, Dinesh Manocha
 * Dept. of Computer Science
 * 201 S. Columbia St.
 * Frederick P. Brooks, Jr. Computer Science Bldg.
 * Chapel Hill, N.C. 27599-3175
 * United States of America
 *
 * <https://gamma.cs.unc.edu/RVO2/>
 */

/**
 * @file  Line.cc
 * @brief Defines the Line class.
 */

#include "Line.h"

namespace RVO {
Line::Line() {}
} /* namespace RVO */
/*
 * Obstacle.cc
 * RVO2 Library
 *
 * SPDX-FileCopyrightText: 2008 University of North Carolina at Chapel Hill
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please send all bug reports to <geom@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Jur van den Berg, Stephen J. Guy, Jamie Snape, Ming C. Lin, Dinesh Manocha
 * Dept. of Computer Science
 * 201 S. Columbia St.
 * Frederick P. Brooks, Jr. Computer Science Bldg.
 * Chapel Hill, N.C. 27599-3175
 * United States of America
 *
 * <https://gamma.cs.unc.edu/RVO2/>
 */

/**
 * @file  Obstacle.cc
 * @brief Defines the Obstacle class.
 */

#include "Obstacle.h"

namespace RVO {
Obstacle::Obstacle()
    : next_(NULL), previous_(NULL), id_(0U), isConvex_(false) {}

Obstacle::~Obstacle() {}
} /* namespace RVO */
/*
 * Agent.cc
 * RVO2 Li/*
 * RVOSimulator.cc
 * RVO2 Library
 *
 * SPDX-FileCopyrightText: 2008 University of North Carolina at Chapel Hill
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please send all bug reports to <geom@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Jur van den Berg, Stephen J. Guy, Jamie Snape, Ming C. Lin, Dinesh Manocha
 * Dept. of Computer Science
 * 201 S. Columbia St.
 * Frederick P. Brooks, Jr. Computer Science Bldg.
 * Chapel Hill, N.C. 27599-3175
 * United States of America
 *
 * <https://gamma.cs.unc.edu/RVO2/>
 */

/**
 * @file  RVOSimulator.cc
 * @brief Defines the RVOSimulator class.
 */

#include "RVOSimulator.h"

#include <limits>
#include <utility>

#include "Agent.h"
#include "KdTree.h"
#include "Line.h"
#include "Obstacle.h"
#include "Vector2.h"

#ifdef _OPENMP
#include <omp.h>
#endif /* _OPENMP */

namespace RVO {
const std::size_t RVO_ERROR = std::numeric_limits<std::size_t>::max();

RVOSimulator::RVOSimulator()
    : defaultAgent_(NULL),
      kdTree_(new KdTree(this)),
      globalTime_(0.0F),
      timeStep_(0.0F) {}

RVOSimulator::RVOSimulator(float timeStep, float neighborDist,
                           std::size_t maxNeighbors, float timeHorizon,
                           float timeHorizonObst, float radius, float maxSpeed)
    : defaultAgent_(new Agent()),
      kdTree_(new KdTree(this)),
      globalTime_(0.0F),
      timeStep_(timeStep) {
  defaultAgent_->maxNeighbors_ = maxNeighbors;
  defaultAgent_->maxSpeed_ = maxSpeed;
  defaultAgent_->neighborDist_ = neighborDist;
  defaultAgent_->radius_ = radius;
  defaultAgent_->timeHorizon_ = timeHorizon;
  defaultAgent_->timeHorizonObst_ = timeHorizonObst;
}

RVOSimulator::RVOSimulator(float timeStep, float neighborDist,
                           std::size_t maxNeighbors, float timeHorizon,
                           float timeHorizonObst, float radius, float maxSpeed,
                           const Vector2 &velocity)
    : defaultAgent_(new Agent()),
      kdTree_(new KdTree(this)),
      globalTime_(0.0F),
      timeStep_(timeStep) {
  defaultAgent_->velocity_ = velocity;
  defaultAgent_->maxNeighbors_ = maxNeighbors;
  defaultAgent_->maxSpeed_ = maxSpeed;
  defaultAgent_->neighborDist_ = neighborDist;
  defaultAgent_->radius_ = radius;
  defaultAgent_->timeHorizon_ = timeHorizon;
  defaultAgent_->timeHorizonObst_ = timeHorizonObst;
}

RVOSimulator::~RVOSimulator() {
  delete defaultAgent_;
  delete kdTree_;

  for (std::size_t i = 0U; i < agents_.size(); ++i) {
    delete agents_[i];
  }

  for (std::size_t i = 0U; i < obstacles_.size(); ++i) {
    delete obstacles_[i];
  }
}

std::size_t RVOSimulator::addAgent(const Vector2 &position) {
  if (defaultAgent_ != NULL) {
    Agent *const agent = new Agent();
    agent->position_ = position;
    agent->velocity_ = defaultAgent_->velocity_;
    agent->id_ = agents_.size();
    agent->maxNeighbors_ = defaultAgent_->maxNeighbors_;
    agent->maxSpeed_ = defaultAgent_->maxSpeed_;
    agent->neighborDist_ = defaultAgent_->neighborDist_;
    agent->radius_ = defaultAgent_->radius_;
    agent->timeHorizon_ = defaultAgent_->timeHorizon_;
    agent->timeHorizonObst_ = defaultAgent_->timeHorizonObst_;
    agents_.push_back(agent);

    return agents_.size() - 1U;
  }

  return RVO_ERROR;
}

std::size_t RVOSimulator::addAgent(const Vector2 &position, float neighborDist,
                                   std::size_t maxNeighbors, float timeHorizon,
                                   float timeHorizonObst, float radius,
                                   float maxSpeed) {
  return addAgent(position, neighborDist, maxNeighbors, timeHorizon,
                  timeHorizonObst, radius, maxSpeed, Vector2());
}

std::size_t RVOSimulator::addAgent(const Vector2 &position, float neighborDist,
                                   std::size_t maxNeighbors, float timeHorizon,
                                   float timeHorizonObst, float radius,
                                   float maxSpeed, const Vector2 &velocity) {
  Agent *const agent = new Agent();
  agent->position_ = position;
  agent->velocity_ = velocity;
  agent->id_ = agents_.size();
  agent->maxNeighbors_ = maxNeighbors;
  agent->maxSpeed_ = maxSpeed;
  agent->neighborDist_ = neighborDist;
  agent->radius_ = radius;
  agent->timeHorizon_ = timeHorizon;
  agent->timeHorizonObst_ = timeHorizonObst;
  agents_.push_back(agent);

  return agents_.size() - 1U;
}

std::size_t RVOSimulator::addObstacle(const std::vector<Vector2> &vertices) {
  if (vertices.size() > 1U) {
    const std::size_t obstacleNo = obstacles_.size();

    for (std::size_t i = 0U; i < vertices.size(); ++i) {
      Obstacle *const obstacle = new Obstacle();
      obstacle->point_ = vertices[i];

      if (i != 0U) {
        obstacle->previous_ = obstacles_.back();
        obstacle->previous_->next_ = obstacle;
      }

      if (i == vertices.size() - 1U) {
        obstacle->next_ = obstacles_[obstacleNo];
        obstacle->next_->previous_ = obstacle;
      }

      obstacle->direction_ = normalize(
          vertices[(i == vertices.size() - 1U ? 0U : i + 1U)] - vertices[i]);

      if (vertices.size() == 2U) {
        obstacle->isConvex_ = true;
      } else {
        obstacle->isConvex_ =
            leftOf(vertices[i == 0U ? vertices.size() - 1U : i - 1U],
                   vertices[i],
                   vertices[i == vertices.size() - 1U ? 0U : i + 1U]) >= 0.0F;
      }

      obstacle->id_ = obstacles_.size();

      obstacles_.push_back(obstacle);
    }

    return obstacleNo;
  }

  return RVO_ERROR;
}

void RVOSimulator::doStep() {
  kdTree_->buildAgentTree();

#ifdef _OPENMP
#pragma omp parallel for
#endif /* _OPENMP */
  for (int i = 0; i < static_cast<int>(agents_.size()); ++i) {
    agents_[i]->computeNeighbors(kdTree_);
    agents_[i]->computeNewVelocity(timeStep_);
  }

#ifdef _OPENMP
#pragma omp parallel for
#endif /* _OPENMP */
  for (int i = 0; i < static_cast<int>(agents_.size()); ++i) {
    agents_[i]->update(timeStep_);
  }

  globalTime_ += timeStep_;
}

std::size_t RVOSimulator::getAgentAgentNeighbor(std::size_t agentNo,
                                                std::size_t neighborNo) const {
  return agents_[agentNo]->agentNeighbors_[neighborNo].second->id_;
}

std::size_t RVOSimulator::getAgentMaxNeighbors(std::size_t agentNo) const {
  return agents_[agentNo]->maxNeighbors_;
}

float RVOSimulator::getAgentMaxSpeed(std::size_t agentNo) const {
  return agents_[agentNo]->maxSpeed_;
}

float RVOSimulator::getAgentNeighborDist(std::size_t agentNo) const {
  return agents_[agentNo]->neighborDist_;
}

std::size_t RVOSimulator::getAgentNumAgentNeighbors(std::size_t agentNo) const {
  return agents_[agentNo]->agentNeighbors_.size();
}

std::size_t RVOSimulator::getAgentNumObstacleNeighbors(
    std::size_t agentNo) const {
  return agents_[agentNo]->obstacleNeighbors_.size();
}

std::size_t RVOSimulator::getAgentNumORCALines(std::size_t agentNo) const {
  return agents_[agentNo]->orcaLines_.size();
}

std::size_t RVOSimulator::getAgentObstacleNeighbor(
    std::size_t agentNo, std::size_t neighborNo) const {
  return agents_[agentNo]->obstacleNeighbors_[neighborNo].second->id_;
}

const Line &RVOSimulator::getAgentORCALine(std::size_t agentNo,
                                           std::size_t lineNo) const {
  return agents_[agentNo]->orcaLines_[lineNo];
}

const Vector2 &RVOSimulator::getAgentPosition(std::size_t agentNo) const {
  return agents_[agentNo]->position_;
}

const Vector2 &RVOSimulator::getAgentPrefVelocity(std::size_t agentNo) const {
  return agents_[agentNo]->prefVelocity_;
}

float RVOSimulator::getAgentRadius(std::size_t agentNo) const {
  return agents_[agentNo]->radius_;
}

float RVOSimulator::getAgentTimeHorizon(std::size_t agentNo) const {
  return agents_[agentNo]->timeHorizon_;
}

float RVOSimulator::getAgentTimeHorizonObst(std::size_t agentNo) const {
  return agents_[agentNo]->timeHorizonObst_;
}

const Vector2 &RVOSimulator::getAgentVelocity(std::size_t agentNo) const {
  return agents_[agentNo]->velocity_;
}

const Vector2 &RVOSimulator::getObstacleVertex(std::size_t vertexNo) const {
  return obstacles_[vertexNo]->point_;
}

std::size_t RVOSimulator::getNextObstacleVertexNo(std::size_t vertexNo) const {
  return obstacles_[vertexNo]->next_->id_;
}

std::size_t RVOSimulator::getPrevObstacleVertexNo(std::size_t vertexNo) const {
  return obstacles_[vertexNo]->previous_->id_;
}

void RVOSimulator::processObstacles() { kdTree_->buildObstacleTree(); }

bool RVOSimulator::queryVisibility(const Vector2 &point1,
                                   const Vector2 &point2) const {
  return kdTree_->queryVisibility(point1, point2, 0.0F);
}

bool RVOSimulator::queryVisibility(const Vector2 &point1, const Vector2 &point2,
                                   float radius) const {
  return kdTree_->queryVisibility(point1, point2, radius);
}

void RVOSimulator::setAgentDefaults(float neighborDist,
                                    std::size_t maxNeighbors, float timeHorizon,
                                    float timeHorizonObst, float radius,
                                    float maxSpeed) {
  setAgentDefaults(neighborDist, maxNeighbors, timeHorizon, timeHorizonObst,
                   radius, maxSpeed, Vector2());
}

void RVOSimulator::setAgentDefaults(float neighborDist,
                                    std::size_t maxNeighbors, float timeHorizon,
                                    float timeHorizonObst, float radius,
                                    float maxSpeed, const Vector2 &velocity) {
  if (defaultAgent_ == NULL) {
    defaultAgent_ = new Agent();
  }

  defaultAgent_->maxNeighbors_ = maxNeighbors;
  defaultAgent_->maxSpeed_ = maxSpeed;
  defaultAgent_->neighborDist_ = neighborDist;
  defaultAgent_->radius_ = radius;
  defaultAgent_->timeHorizon_ = timeHorizon;
  defaultAgent_->timeHorizonObst_ = timeHorizonObst;
  defaultAgent_->velocity_ = velocity;
}

void RVOSimulator::setAgentMaxNeighbors(std::size_t agentNo,
                                        std::size_t maxNeighbors) {
  agents_[agentNo]->maxNeighbors_ = maxNeighbors;
}

void RVOSimulator::setAgentMaxSpeed(std::size_t agentNo, float maxSpeed) {
  agents_[agentNo]->maxSpeed_ = maxSpeed;
}

void RVOSimulator::setAgentNeighborDist(std::size_t agentNo,
                                        float neighborDist) {
  agents_[agentNo]->neighborDist_ = neighborDist;
}

void RVOSimulator::setAgentPosition(std::size_t agentNo,
                                    const Vector2 &position) {
  agents_[agentNo]->position_ = position;
}

void RVOSimulator::setAgentPrefVelocity(std::size_t agentNo,
                                        const Vector2 &prefVelocity) {
  agents_[agentNo]->prefVelocity_ = prefVelocity;
}

void RVOSimulator::setAgentRadius(std::size_t agentNo, float radius) {
  agents_[agentNo]->radius_ = radius;
}

void RVOSimulator::setAgentTimeHorizon(std::size_t agentNo, float timeHorizon) {
  agents_[agentNo]->timeHorizon_ = timeHorizon;
}

void RVOSimulator::setAgentTimeHorizonObst(std::size_t agentNo,
                                           float timeHorizonObst) {
  agents_[agentNo]->timeHorizonObst_ = timeHorizonObst;
}

void RVOSimulator::setAgentVelocity(std::size_t agentNo,
                                    const Vector2 &velocity) {
  agents_[agentNo]->velocity_ = velocity;
}
} /* namespace RVO */
/*
 * Vector2.cpp
 * RVO2 Library
 *
 * SPDX-FileCopyrightText: 2008 University of North Carolina at Chapel Hill
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please send all bug reports to <geom@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Jur van den Berg, Stephen J. Guy, Jamie Snape, Ming C. Lin, Dinesh Manocha
 * Dept. of Computer Science
 * 201 S. Columbia St.
 * Frederick P. Brooks, Jr. Computer Science Bldg.
 * Chapel Hill, N.C. 27599-3175
 * United States of America
 *
 * <https://gamma.cs.unc.edu/RVO2/>
 */

/**
 * @file  Vector2.cc
 * @brief Defines the Vector2 class.
 */

#include "Vector2.h"

#include <cmath>
#include <ostream>

namespace RVO {
const float RVO_EPSILON = 0.00001F;

Vector2::Vector2() : x_(0.0F), y_(0.0F) {}

Vector2::Vector2(float x, float y) : x_(x), y_(y) {}

Vector2 Vector2::operator-() const { return Vector2(-x_, -y_); }

float Vector2::operator*(const Vector2 &vector) const {
  return x_ * vector.x_ + y_ * vector.y_;
}

Vector2 Vector2::operator*(float scalar) const {
  return Vector2(x_ * scalar, y_ * scalar);
}

Vector2 Vector2::operator/(float scalar) const {
  const float invScalar = 1.0F / scalar;

  return Vector2(x_ * invScalar, y_ * invScalar);
}

Vector2 Vector2::operator+(const Vector2 &vector) const {
  return Vector2(x_ + vector.x_, y_ + vector.y_);
}

Vector2 Vector2::operator-(const Vector2 &vector) const {
  return Vector2(x_ - vector.x_, y_ - vector.y_);
}

bool Vector2::operator==(const Vector2 &vector) const {
  return x_ == vector.x_ && y_ == vector.y_;
}

bool Vector2::operator!=(const Vector2 &vector) const {
  return x_ != vector.x_ || y_ != vector.y_;
}

Vector2 &Vector2::operator*=(float scalar) {
  x_ *= scalar;
  y_ *= scalar;

  return *this;
}

Vector2 &Vector2::operator/=(float scalar) {
  const float invScalar = 1.0F / scalar;
  x_ *= invScalar;
  y_ *= invScalar;

  return *this;
}

Vector2 &Vector2::operator+=(const Vector2 &vector) {
  x_ += vector.x_;
  y_ += vector.y_;

  return *this;
}

Vector2 &Vector2::operator-=(const Vector2 &vector) {
  x_ -= vector.x_;
  y_ -= vector.y_;

  return *this;
}

Vector2 operator*(float scalar, const Vector2 &vector) {
  return Vector2(scalar * vector.x(), scalar * vector.y());
}

std::ostream &operator<<(std::ostream &stream, const Vector2 &vector) {
  stream << "(" << vector.x() << "," << vector.y() << ")";

  return stream;
}

float abs(const Vector2 &vector) { return std::sqrt(vector * vector); }

float absSq(const Vector2 &vector) { return vector * vector; }

float det(const Vector2 &vector1, const Vector2 &vector2) {
  return vector1.x() * vector2.y() - vector1.y() * vector2.x();
}

float leftOf(const Vector2 &vector1, const Vector2 &vector2,
             const Vector2 &vector3) {
  return det(vector1 - vector3, vector2 - vector1);
}

Vector2 normalize(const Vector2 &vector) { return vector / abs(vector); }
} /* namespace RVO */
