#pragma once

#include <atomic>

namespace rd_utils::concurrency {

  template<typename T>
  class LockFreeMailbox {
  private:

    // Node of the queue
    struct Node {
      bool contains;
      std::atomic<Node*> next;
      Node() : next (nullptr), contains (false) {}
    };

    struct DataNode : public Node {
      T data;
      DataNode (T && data) : data(std::move (data)) {
        this-> contains = true;
      }
    };

    // Start of the queue
    std::atomic<Node*> _head;

    // End of the queue
    std::atomic<Node*> _tail;

  public:

    /**
     * Create a new empty mailbox
     */
    LockFreeMailbox () {
      Node * n = new Node ();
      this-> _head.exchange (n);
      this-> _tail.exchange (n);
    }

    /**
     * Move ctor
     */
    LockFreeMailbox (LockFreeMailbox<T> && other) {
      this-> _head.exchange (other._head.load ());
      this-> _tail.exchange (other._tail.load ());

      other._head.exchange (new Node ());
      other._tail.exchange (other._head.load ());
    }

    /**
     * Move affectation
     */
    void operator=(LockFreeMailbox<T> && other) {
      this-> dispose ();

      this-> _head.exchange (other._head.load ());
      this-> _tail.exchange (other._tail.load ());
      other._head.exchange (new Node ());
      other._tail.exchange (other._head.load ());
    }

    /**
     * Post a message in the box
     */
    void send (T data) {
      Node * new_node = new DataNode (std::move (data));
      Node * old_tail = this-> _tail.load (std::memory_order_relaxed);
      Node * null = nullptr;

      for (;;) {
        Node * old_tail = this-> _tail.load (std::memory_order_relaxed);
        if (old_tail-> next.compare_exchange_weak (null, new_node)) {
          this-> _tail.compare_exchange_weak (old_tail, new_node);
          return ;
        }
      }
    }

    /**
     * Receive a message from the box
     * @returns: true iif a message was read (into 'val')
     */
    bool receive (T & val) {
      for (;;) {
		Node * old_head = this-> _head.load (std::memory_order_relaxed);
        Node * x = old_head;
        if (this-> _head.compare_exchange_weak (x, old_head)) {
          Node * next = old_head-> next;
          x = old_head;
          if (this-> _tail.compare_exchange_weak (x, old_head)) {
            if (next == nullptr) return false;
          } else {
            if (next != nullptr) {
              x = old_head;
              if (this-> _head.compare_exchange_weak (x, next)) {
                val = ((DataNode*)next)-> data;
                delete old_head;
                return true;
              } else {
                return false;
              }
            }
          }
        }
      }
    }

    /**
     * Remove everything stored in the mailbox
     */
    void dispose () {
      while (this-> pop ());
    }

    /**
     * this-> dispose ();
     */
    ~LockFreeMailbox () {
      this-> dispose ();
      delete this-> _head.load ();
    }

  private:

    /**
     * Pop one element from the queue
     * @returns: true if an element was present, false otherwise
     */
    bool pop () {
     for (;;) {
		Node * old_head = this-> _head.load (std::memory_order_relaxed);
        Node * x = old_head;
        if (this-> _head.compare_exchange_weak (x, old_head)) {
          Node * next = old_head-> next;
          x = old_head;
          if (this-> _tail.compare_exchange_weak (x, old_head)) {
            if (next == nullptr) return false;
          } else {
            if (next != nullptr) {
              x = old_head;
              if (this-> _head.compare_exchange_weak (x, next)) {
                delete old_head;
                return true;
              } else {
                return false;
              }
            }
          }
        }
      }
    }

  };

}
