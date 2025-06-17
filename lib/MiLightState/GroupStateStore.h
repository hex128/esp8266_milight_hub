#pragma once

#include <GroupState.h>
#include <GroupStateCache.h>
#include <GroupStatePersistence.h>

class GroupStateStore {
public:
  GroupStateStore(size_t maxSize, size_t flushRate);

  /*
* Retrieves the state for a given BulbId. For valid devices (groups 1-4), creates and 
* initializes state with defaults if none exists. Returns NULL for invalid devices 
* (group 0).
   *
   * Otherwise, we return NULL.
   */
  GroupState* get(const BulbId& id);
  GroupState* get(uint16_t deviceId, uint8_t groupId, MiLightRemoteType deviceType);

  /*
   * Sets the state for the given BulbId.  State will be marked as dirty and
   * flushed to persistent storage.
   */
  GroupState* set(const BulbId& id, const GroupState& state);
  GroupState* set(uint16_t deviceId, uint8_t groupId, MiLightRemoteType deviceType, const GroupState& state);

  void clear(const BulbId& id);

  /*
   * Flushes all states to persistent storage.  Returns true iff anything was
   * flushed.
   */
  bool flush();

  /*
   * Flushes at most one dirty state to persistent storage.  Rate limit
   * specified by Settings.
   */
  void limitedFlush();

private:
  GroupStateCache cache;
  GroupStatePersistence persistence;
  LinkedList<BulbId> evictedIds;
  const size_t flushRate;
  unsigned long lastFlush;

  void trackEviction();
};
