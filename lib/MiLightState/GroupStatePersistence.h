#pragma once

#include <GroupState.h>

class GroupStatePersistence {
public:
  void get(const BulbId& id, GroupState& state);
  void set(const BulbId& id, const GroupState& state);

  void clear(const BulbId& id);

private:

  static char* buildFilename(const BulbId& id, char* buffer);
};
