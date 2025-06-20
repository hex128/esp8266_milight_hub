#pragma once

#include <GroupState.h>
#include <LinkedList.h>

struct GroupCacheNode {
  GroupCacheNode() {}
  GroupCacheNode(const BulbId& id, const GroupState& state)
    : id(id), state(state) { }

  BulbId id;
  GroupState state;
};

class GroupStateCache {
public:
  explicit GroupStateCache(size_t maxSize);
  ~GroupStateCache();

  GroupState* get(const BulbId& id);
  GroupState* set(const BulbId& id, const GroupState& state);
  BulbId getLru() const;
  bool isFull() const;
  ListNode<GroupCacheNode*>* getHead();

private:
  LinkedList<GroupCacheNode*> cache;
  const size_t maxSize;

  GroupState* getInternal(const BulbId& id);
};
