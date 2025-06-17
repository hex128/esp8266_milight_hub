#include <GroupStateCache.h>

GroupStateCache::GroupStateCache(const size_t maxSize)
  : maxSize(maxSize)
{ }

GroupStateCache::~GroupStateCache() {
  const ListNode<GroupCacheNode*>* cur = cache.getHead();

  while (cur != nullptr) {
    delete cur->data;
    cur = cur->next;
  }
}

GroupState* GroupStateCache::get(const BulbId& id) {
  return getInternal(id);
}

GroupState* GroupStateCache::set(const BulbId& id, const GroupState& state) {
  GroupCacheNode* pushedNode = nullptr;
  if (cache.size() >= maxSize) {
    pushedNode = cache.pop();
  }

  GroupState* cachedState = getInternal(id);

  if (cachedState == nullptr) {
    if (pushedNode == nullptr) {
      const auto newNode = new GroupCacheNode(id, state);
      cachedState = &newNode->state;
      cache.unshift(newNode);
    } else {
      pushedNode->id = id;
      pushedNode->state = state;
      cachedState = &pushedNode->state;
      cache.unshift(pushedNode);
    }
  } else {
    *cachedState = state;
  }

  return cachedState;
}

BulbId GroupStateCache::getLru() const {
  GroupCacheNode* node = cache.getLast();
  return node->id;
}

bool GroupStateCache::isFull() const {
  return cache.size() >= maxSize;
}

ListNode<GroupCacheNode*>* GroupStateCache::getHead() {
  return cache.getHead();
}

GroupState* GroupStateCache::getInternal(const BulbId& id) {
  ListNode<GroupCacheNode*>* cur = cache.getHead();

  while (cur != nullptr) {
    if (cur->data->id == id) {
      GroupState* result = &cur->data->state;
      cache.spliceToFront(cur);
      return result;
    }
    cur = cur->next;
  }

  return nullptr;
}
