#pragma once

#include <GroupState.h>

class GroupStatePersistence {
public:
  static void get(const BulbId& id, GroupState& state);
  static void set(const BulbId& id, const GroupState& state);

  static void clear(const BulbId& id);

private:

  static char* buildFilename(const BulbId& id, char* buffer);
};
