/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  ------------------------------------------------------------------------------*/
#include "string_utilities.hpp"
#include "debug.hpp"
#include <vector>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
// ntree_node

template<typename T>
class ntree_node
{
public:
  T m_data;
  ntree_node<T>* m_parent;
  std::vector<ntree_node<T>*> m_children;
public:
  ntree_node(const T& d = T()) : m_data(d), m_parent(0)
    {
    }

  ~ntree_node(void)
    {
      m_parent = (ntree_node<T>*)1;
      for (typename std::vector<ntree_node<T>*>::iterator i = m_children.begin(); i != m_children.end(); i++)
        delete *i;
    }

};

template<typename T>
static ntree_node<T>* ntree_copy(ntree_node<T>* root)
{
  if (!root) return 0;
  ntree_node<T>* new_tree = new ntree_node<T>(root->m_data);
  for (typename std::vector<ntree_node<T>*>::iterator i = root->m_children.begin(); i != root->m_children.end(); i++)
  {
    ntree_node<T>* new_child = ntree_copy(*i);
    new_tree->m_children.push_back(new_child);
    new_child->m_parent = new_tree;
  }
  return new_tree;
}

template<typename T>
static unsigned ntree_size(ntree_node<T>* root)
{
  if (!root) return 0;
  unsigned result = 1;
  for (typename std::vector<ntree_node<T>*>::iterator i = root->m_children.begin(); i != root->m_children.end(); i++)
    result += ntree_size(*i);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// DJDM added comparison operators
// less-than is defined only in terms of less-than for the template type, no other operators on T are required

template<typename T>
bool ntree_less(const ntree_node<T>& left, const ntree_node<T>& right)
{
  // compare the node contents
  // as with the map, test for inequality by finding less-than or greater-than
  bool less = left.m_data < right.m_data;
  if (less != (right.m_data < left.m_data)) return less;
  // compare the number of children - a small tree is less than a large tree
  unsigned lsize = left.m_children.size();
  unsigned rsize = right.m_children.size();
  if (lsize != rsize) return lsize < rsize;
  // now recurse on the children to see if a difference can be found
  // the ordering is based on the first different child
  for (unsigned i = 0; i < lsize; i++)
  {
    less = ntree_less(left.m_children[i], right.m_children[i]);
    if (less != ntree_less(right.m_children[i], left.m_children[i])) return less;
  }
  return false;
}

template<typename T>
bool ntree_less(const ntree_node<T>* left, const ntree_node<T>* right)
{
  // if both pointers are non-null, compare the contents, otherwise compare the pointers
  if (left && right) return ntree_less(*left,*right);
  return left < right;
}

template<typename T>
bool ntree_equal(const ntree_node<T>& left, const ntree_node<T>& right)
{
  // compare the node contents
  // if different then the trees are different
  if (!(left.m_data == right.m_data)) return false;
  // compare the number of children
  // if different then the trees are different
  unsigned lsize = left.m_children.size();
  if (lsize != right.m_children.size()) return false;
  // now recurse on the children looking for a difference
  // if any child is different, then the trees are different
  for (unsigned i = 0; i < lsize; i++)
    if (!ntree_equal(left.m_children[i], right.m_children[i])) return false;
  // no differences were found, so the trees must be equal
  return true;
}

template<typename T>
bool ntree_equal(const ntree_node<T>* left, const ntree_node<T>* right)
{
  // if both pointers are non-null, compare the contents, otherwise compare the pointers
  if (left && right) return ntree_equal(*left,*right);
  return left == right;
}

////////////////////////////////////////////////////////////////////////////////

template<typename T>
static otext& ntree_print(otext& str, ntree_node<T>* root, unsigned indent)
{
  print_indent(str, indent);
  str << (void*)root;
  if (root)
  {
    str << " {" << (void*)(root->m_parent) << ":";
    for (typename std::vector<ntree_node<T>*>::iterator i = root->m_children.begin(); i != root->m_children.end(); i++)
    {
      if (i != root->m_children.begin()) str << ",";
      str << (void*)(*i);
    }
    str << "}";
  }
  str << endl;
  if (root)
    for (typename std::vector<ntree_node<T>*>::iterator i = root->m_children.begin(); i != root->m_children.end(); i++)
      ntree_print(str, *i, indent+1);
  return str;
}

////////////////////////////////////////////////////////////////////////////////
// ntree_iterator

template<typename T, typename TRef, typename TPtr>
ntree_iterator<T,TRef,TPtr>::ntree_iterator(void) :
  m_owner(0), m_node(0)
{
}

template<typename T, typename TRef, typename TPtr>
ntree_iterator<T,TRef,TPtr>::~ntree_iterator(void)
{
}

template<typename T, typename TRef, typename TPtr>
ntree_iterator<T,TRef,TPtr>::ntree_iterator(const ntree<T>* owner, ntree_node<T>* node) :
  m_owner(owner), m_node(node)
{
}

template<typename T, typename TRef, typename TPtr>
bool ntree_iterator<T,TRef,TPtr>::null (void) const
{
  return m_owner == 0;
}

template<typename T, typename TRef, typename TPtr>
bool ntree_iterator<T,TRef,TPtr>::end (void) const
{
  if (m_owner == 0) return false;
  return m_node == 0;
}

template<typename T, typename TRef, typename TPtr>
bool ntree_iterator<T,TRef,TPtr>::valid (void) const
{
  return !null() && !end();
}

template<typename T, typename TRef, typename TPtr>
const ntree<T>* ntree_iterator<T,TRef,TPtr>::owner(void) const
{
  return m_owner;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_iterator<T,TRef,TPtr>::const_iterator ntree_iterator<T,TRef,TPtr>::constify(void) const
{
  return ntree_iterator<T,TRef,TPtr>::const_iterator(m_owner,m_node);
}

template<typename T, typename TRef, typename TPtr>
typename ntree_iterator<T,TRef,TPtr>::iterator ntree_iterator<T,TRef,TPtr>::deconstify(void) const
{
  return ntree_iterator<T,TRef,TPtr>::iterator(m_owner,m_node);
}

template<typename T, typename TRef, typename TPtr>
bool ntree_iterator<T,TRef,TPtr>::operator == (const PARAMETER_TYPENAME ntree_iterator<T,TRef,TPtr>::this_iterator& r) const
{
  return m_node == r.m_node;
}

template<typename T, typename TRef, typename TPtr>
bool ntree_iterator<T,TRef,TPtr>::operator != (const PARAMETER_TYPENAME ntree_iterator<T,TRef,TPtr>::this_iterator& r) const
{
  return m_node != r.m_node;
}

template<typename T, typename TRef, typename TPtr>
bool ntree_iterator<T,TRef,TPtr>::operator < (const PARAMETER_TYPENAME ntree_iterator<T,TRef,TPtr>::this_iterator& r) const
{
  return m_node < r.m_node;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_iterator<T,TRef,TPtr>::reference ntree_iterator<T,TRef,TPtr>::operator*(void) const
  throw(null_dereference,end_dereference)
{
  check_valid();
  return m_node->m_data;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_iterator<T,TRef,TPtr>::pointer ntree_iterator<T,TRef,TPtr>::operator->(void) const
  throw(null_dereference,end_dereference)
{
  check_valid();
  return &(m_node->m_data);
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::dump(dump_context& context) const throw(persistent_dump_failed)
{
  // dump the magic keys to both the owner pointer and the node pointer
  // if either is not already registered with the dump context, throw an exception
  dump_xref(context,m_owner);
  dump_xref(context,m_node);
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::restore(restore_context& context) throw(persistent_restore_failed)
{
  m_owner = 0;
  m_node = 0;
  restore_xref(context,m_owner);
  restore_xref(context,m_node);
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::check_owner(const ntree<T>* owner) const
  throw(wrong_object)
{
  if (owner != m_owner)
    throw wrong_object("ntree node iterator");
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::check_non_null(void) const
  throw(null_dereference)
{
  if (null())
    throw null_dereference("ntree node iterator");
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::check_non_end(void) const
  throw(end_dereference)
{
  if (end())
    throw end_dereference("ntree node iterator");
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::check_valid(void) const
  throw(null_dereference,end_dereference)
{
  check_non_null();
  check_non_end();
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::check(const ntree<T>* owner) const
  throw(wrong_object,null_dereference,end_dereference)
{
  check_valid();
  if (owner) check_owner(owner);
}

template<typename T, typename TRef, typename TPtr>
void dump_ntree_iterator(dump_context& context, const ntree_iterator<T,TRef,TPtr>& data) throw(persistent_dump_failed)
{
  data.dump(context);
}

template<typename T, typename TRef, typename TPtr>
void restore_ntree_iterator(restore_context& context, ntree_iterator<T,TRef,TPtr>& data) throw(persistent_restore_failed)
{
  data.restore(context);
}

////////////////////////////////////////////////////////////////////////////////
// ntree_prefix_iterator

template<typename T, typename TRef, typename TPtr>
ntree_prefix_iterator<T,TRef,TPtr>::ntree_prefix_iterator(void)
{
}

template<typename T, typename TRef, typename TPtr>
ntree_prefix_iterator<T,TRef,TPtr>::~ntree_prefix_iterator(void)
{
}

template<typename T, typename TRef, typename TPtr>
ntree_prefix_iterator<T,TRef,TPtr>::ntree_prefix_iterator(const ntree_iterator<T,TRef,TPtr>& i) :
  m_iterator(i)
{
  // this is initialised with the root node
  // which is also the first node in prefix traversal order
}

template<typename T, typename TRef, typename TPtr>
typename ntree_prefix_iterator<T,TRef,TPtr>::const_iterator ntree_prefix_iterator<T,TRef,TPtr>::constify(void) const
{
  return ntree_prefix_iterator<T,TRef,TPtr>::const_iterator(m_iterator);
}

template<typename T, typename TRef, typename TPtr>
typename ntree_prefix_iterator<T,TRef,TPtr>::iterator ntree_prefix_iterator<T,TRef,TPtr>::deconstify(void) const
{
  return ntree_prefix_iterator<T,TRef,TPtr>::iterator(m_iterator);
}

template<typename T, typename TRef, typename TPtr>
ntree_iterator<T,TRef,TPtr> ntree_prefix_iterator<T,TRef,TPtr>::simplify(void) const
{
  return ntree_iterator<T,TRef,TPtr>(m_iterator.m_owner,m_iterator.m_node);
}

template<typename T, typename TRef, typename TPtr>
bool ntree_prefix_iterator<T,TRef,TPtr>::operator == (const PARAMETER_TYPENAME ntree_prefix_iterator<T,TRef,TPtr>::this_iterator& r) const
{
  return m_iterator == r.m_iterator;
}

template<typename T, typename TRef, typename TPtr>
bool ntree_prefix_iterator<T,TRef,TPtr>::operator != (const PARAMETER_TYPENAME ntree_prefix_iterator<T,TRef,TPtr>::this_iterator& r) const
{
  return m_iterator != r.m_iterator;
}

template<typename T, typename TRef, typename TPtr>
bool ntree_prefix_iterator<T,TRef,TPtr>::operator < (const PARAMETER_TYPENAME ntree_prefix_iterator<T,TRef,TPtr>::this_iterator& r) const
{
  return m_iterator < r.m_iterator;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_prefix_iterator<T,TRef,TPtr>::this_iterator& ntree_prefix_iterator<T,TRef,TPtr>::operator ++ (void)
  throw(null_dereference,end_dereference)
{
  // pre-increment operator
  // algorithm: if there are any children, visit child 0, otherwise, go to
  // parent and deduce which child the start node was of that parent - if
  // there are further children, go into the next one. Otherwise, go up the
  // tree and test again for further children. Return null if there are no
  // further nodes
  m_iterator.check_valid();
  if (!m_iterator.m_node->m_children.empty())
  {
    // simply take the first child of this node
    m_iterator.m_node = m_iterator.m_node->m_children[0];
    return *this;
  }
  // this loop walks up the parent pointers
  // either it will walk off the top and exit or a new node will be found and the loop will exit
  for (;;)
  {
    // go up a level
    ntree_node<T>* old_node = m_iterator.m_node;
    m_iterator.m_node = m_iterator.m_node->m_parent;
    // if we've walked off the top of the tree, then return null
    if (!m_iterator.m_node) return *this;
    // otherwise find which index the old node was relative to this node
    typename std::vector<ntree_node<T>*>::iterator found = 
      std::find(m_iterator.m_node->m_children.begin(), m_iterator.m_node->m_children.end(), old_node);
    DEBUG_ASSERT(found != m_iterator.m_node->m_children.end());
    // if this was found, then see if there is another and if so return that
    found++;
    if (found != m_iterator.m_node->m_children.end())
    {
      m_iterator.m_node = *found;
      return *this;
    }
  }
  return *this;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_prefix_iterator<T,TRef,TPtr>::this_iterator ntree_prefix_iterator<T,TRef,TPtr>::operator ++ (int)
  throw(null_dereference,end_dereference)
{
  // post-increment is defined in terms of the pre-increment
  ntree_prefix_iterator<T,TRef,TPtr>::this_iterator result = *this;
  ++(*this);
  return result;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_prefix_iterator<T,TRef,TPtr>::reference ntree_prefix_iterator<T,TRef,TPtr>::operator*(void) const
  throw(null_dereference,end_dereference)
{
  return m_iterator.operator*();
}

template<typename T, typename TRef, typename TPtr>
typename ntree_prefix_iterator<T,TRef,TPtr>::pointer ntree_prefix_iterator<T,TRef,TPtr>::operator->(void) const
  throw(null_dereference,end_dereference)
{
  return m_iterator.operator->();
}

template<typename T, typename TRef, typename TPtr>
void ntree_prefix_iterator<T,TRef,TPtr>::dump(dump_context& context) const throw(persistent_dump_failed)
{
  m_iterator.dump(context);
}

template<typename T, typename TRef, typename TPtr>
void ntree_prefix_iterator<T,TRef,TPtr>::restore(restore_context& context) throw(persistent_restore_failed)
{
  m_iterator.restore(context);
}

template<typename T, typename TRef, typename TPtr>
void dump_ntree_prefix_iterator(dump_context& context, const ntree_prefix_iterator<T,TRef,TPtr>& data) throw(persistent_dump_failed)
{
  data.dump(context);
}

template<typename T, typename TRef, typename TPtr>
void restore_ntree_prefix_iterator(restore_context& context, ntree_prefix_iterator<T,TRef,TPtr>& data) throw(persistent_restore_failed)
{
  data.restore(context);
}

////////////////////////////////////////////////////////////////////////////////
// ntree_postfix_iterator

template<typename T, typename TRef, typename TPtr>
ntree_postfix_iterator<T,TRef,TPtr>::ntree_postfix_iterator(void)
{
}

template<typename T, typename TRef, typename TPtr>
ntree_postfix_iterator<T,TRef,TPtr>::~ntree_postfix_iterator(void)
{
}

template<typename T, typename TRef, typename TPtr>
ntree_postfix_iterator<T,TRef,TPtr>::ntree_postfix_iterator(const ntree_iterator<T,TRef,TPtr>& i) :
  m_iterator(i)
{
  // this is initialised with the root node
  // initially traverse to the first node to be visited
  if (m_iterator.m_node)
    while (!m_iterator.m_node->m_children.empty())
      m_iterator.m_node = m_iterator.m_node->m_children[0];
}

template<typename T, typename TRef, typename TPtr>
typename ntree_postfix_iterator<T,TRef,TPtr>::const_iterator ntree_postfix_iterator<T,TRef,TPtr>::constify(void) const
{
  return ntree_postfix_iterator<T,TRef,TPtr>::const_iterator(m_iterator);
}

template<typename T, typename TRef, typename TPtr>
typename ntree_postfix_iterator<T,TRef,TPtr>::iterator ntree_postfix_iterator<T,TRef,TPtr>::deconstify(void) const
{
  return ntree_postfix_iterator<T,TRef,TPtr>::iterator(m_iterator);
}

template<typename T, typename TRef, typename TPtr>
ntree_iterator<T,TRef,TPtr> ntree_postfix_iterator<T,TRef,TPtr>::simplify(void) const
{
  return ntree_iterator<T,TRef,TPtr>(m_iterator.m_owner,m_iterator.m_node);
}

template<typename T, typename TRef, typename TPtr>
bool ntree_postfix_iterator<T,TRef,TPtr>::operator == (const PARAMETER_TYPENAME ntree_postfix_iterator<T,TRef,TPtr>::this_iterator& r) const
{
  return m_iterator == r.m_iterator;
}

template<typename T, typename TRef, typename TPtr>
bool ntree_postfix_iterator<T,TRef,TPtr>::operator != (const PARAMETER_TYPENAME ntree_postfix_iterator<T,TRef,TPtr>::this_iterator& r) const
{
  return m_iterator != r.m_iterator;
}

template<typename T, typename TRef, typename TPtr>
bool ntree_postfix_iterator<T,TRef,TPtr>::operator < (const PARAMETER_TYPENAME ntree_postfix_iterator<T,TRef,TPtr>::this_iterator& r) const
{
  return m_iterator < r.m_iterator;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_postfix_iterator<T,TRef,TPtr>::this_iterator& ntree_postfix_iterator<T,TRef,TPtr>::operator ++ (void)
  throw(null_dereference,end_dereference)
{
  // pre-increment operator
  // algorithm: this node has been visited, therefore all children must have
  // already been visited. So go to parent. Return null if the parent is null.
  // Otherwise deduce which child the start node was of that parent - if there
  // are further children, go into the next one and then walk down any
  // subsequent first-child pointers to the bottom. Otherwise, if there are no
  // children then the parent node is the next in the traversal.
  m_iterator.check_valid();
  if (!m_iterator.m_node) return *this;
  // go up a level
  ntree_node<T>* old_node = m_iterator.m_node;
  m_iterator.m_node = m_iterator.m_node->m_parent;
  // if we've walked off the top of the tree, then the result is null so there's nothing more to be done
  if (!m_iterator.m_node) return *this;
  // otherwise find which index the old node was relative to this node
  typename std::vector<ntree_node<T>*>::iterator found =
    std::find(m_iterator.m_node->m_children.begin(), m_iterator.m_node->m_children.end(), old_node);
  DEBUG_ASSERT(found != m_iterator.m_node->m_children.end());
  // if this was found, then see if there is another - if not then the current node is the next in the iteration
  found++;
  if (found == m_iterator.m_node->m_children.end()) return *this;
  // if so traverse to it
  m_iterator.m_node = *found;
  // now walk down the leftmost child pointers to the bottom of the new sub-tree
  while (!m_iterator.m_node->m_children.empty())
    m_iterator.m_node = m_iterator.m_node->m_children[0];
  return *this;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_postfix_iterator<T,TRef,TPtr>::this_iterator ntree_postfix_iterator<T,TRef,TPtr>::operator ++ (int)
  throw(null_dereference,end_dereference)
{
  // post-increment is defined in terms of the pre-increment
  ntree_postfix_iterator<T,TRef,TPtr>::this_iterator result = *this;
  ++(*this);
  return result;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_postfix_iterator<T,TRef,TPtr>::reference ntree_postfix_iterator<T,TRef,TPtr>::operator*(void) const
  throw(null_dereference,end_dereference)
{
  return m_iterator.operator*();
}

template<typename T, typename TRef, typename TPtr>
typename ntree_postfix_iterator<T,TRef,TPtr>::pointer ntree_postfix_iterator<T,TRef,TPtr>::operator->(void) const
  throw(null_dereference,end_dereference)
{
  return m_iterator.operator->();
}

template<typename T, typename TRef, typename TPtr>
void ntree_postfix_iterator<T,TRef,TPtr>::dump(dump_context& context) const throw(persistent_dump_failed)
{
  m_iterator.dump(context);
}

template<typename T, typename TRef, typename TPtr>
void ntree_postfix_iterator<T,TRef,TPtr>::restore(restore_context& context) throw(persistent_restore_failed)
{
  m_iterator.restore(context);
}

template<typename T, typename TRef, typename TPtr>
void dump_ntree_postfix_iterator(dump_context& context, const ntree_postfix_iterator<T,TRef,TPtr>& data) throw(persistent_dump_failed)
{
  data.dump(context);
}

template<typename T, typename TRef, typename TPtr>
void restore_ntree_postfix_iterator(restore_context& context, ntree_postfix_iterator<T,TRef,TPtr>& data) throw(persistent_restore_failed)
{
  data.restore(context);
}

////////////////////////////////////////////////////////////////////////////////
// ntree

template<typename T>
ntree<T>::ntree(void) : m_root(0)
{
}

template<typename T>
ntree<T>::~ntree(void)
{
  delete m_root;
}

template<typename T>
ntree<T>::ntree(const ntree<T>& r) : m_root(0)
{
  *this = r;
}

template<typename T>
ntree<T>& ntree<T>::operator=(const ntree<T>& r)
{
  delete m_root;
  m_root = ntree_copy(r.m_root);
  return *this;
}

template<typename T>
bool ntree<T>::empty(void) const
{
  return m_root == 0;
}

template<typename T>
unsigned ntree<T>::size(void) const
{
  return ntree_size(m_root);
}

template<typename T>
unsigned ntree<T>::size(PARAMETER_TYPENAME ntree<T>::const_iterator i) const
  throw(wrong_object,null_dereference,end_dereference)
{
  i.check(this);
  return ntree_size(i.m_node);
}

template<typename T>
unsigned ntree<T>::size(PARAMETER_TYPENAME ntree<T>::iterator i)
  throw(wrong_object,null_dereference,end_dereference)
{
  i.check(this);
  return ntree_size(i.m_node);
}

template<typename T>
typename ntree<T>::const_iterator ntree<T>::root(void) const
{
  return ntree<T>::const_iterator(this,m_root);
}

template<typename T>
typename ntree<T>::iterator ntree<T>::root(void)
{
  return ntree<T>::iterator(this,m_root);
}

template<typename T>
unsigned ntree<T>::children(PARAMETER_TYPENAME ntree<T>::const_iterator i) const
  throw(wrong_object,null_dereference,end_dereference)
{
  i.check(this);
  return i.m_node->m_children.size();
}

template<typename T>
unsigned ntree<T>::children(PARAMETER_TYPENAME ntree<T>::iterator i)
  throw(wrong_object,null_dereference,end_dereference)
{
  i.check(this);
  return i.m_node->m_children.size();
}

template<typename T>
typename ntree<T>::const_iterator ntree<T>::child(PARAMETER_TYPENAME ntree<T>::const_iterator i, unsigned child) const
  throw(wrong_object,null_dereference,end_dereference,std::out_of_range)
{
  i.check(this);
  if (child >= children(i))
    throw std::out_of_range("ntree");
  return ntree<T>::const_iterator(this,i.m_node->m_children[child]);
}

template<typename T>
typename ntree<T>::iterator ntree<T>::child(PARAMETER_TYPENAME ntree<T>::iterator i, unsigned child)
  throw(wrong_object,null_dereference,end_dereference,std::out_of_range)
{
  i.check(this);
  if (child >= children(i))
    throw std::out_of_range("ntree");
  return ntree<T>::iterator(this,i.m_node->m_children[child]);
}

template<typename T>
typename ntree<T>::const_iterator ntree<T>::parent(PARAMETER_TYPENAME ntree<T>::const_iterator i) const
  throw(wrong_object,null_dereference,end_dereference)
{
  i.check(this);
  return ntree<T>::const_iterator(this,i.m_node->m_parent);
}

template<typename T>
typename ntree<T>::iterator ntree<T>::parent(PARAMETER_TYPENAME ntree<T>::iterator i)
  throw(wrong_object,null_dereference,end_dereference)
{
  i.check(this);
  return ntree<T>::iterator(this,i.m_node->m_parent);
}

template<typename T>
typename ntree<T>::const_prefix_iterator ntree<T>::prefix_begin(void) const
{
  return ntree<T>::const_prefix_iterator(ntree<T>::const_iterator(this,m_root));
}

template<typename T>
typename ntree<T>::prefix_iterator ntree<T>::prefix_begin(void)
{
  return ntree<T>::prefix_iterator(ntree<T>::iterator(this,m_root));
}

template<typename T>
typename ntree<T>::const_prefix_iterator ntree<T>::prefix_end(void) const
{
  return ntree<T>::const_prefix_iterator(ntree<T>::const_iterator(this,0));
}

template<typename T>
typename ntree<T>::prefix_iterator ntree<T>::prefix_end(void)
{
  return ntree<T>::prefix_iterator(ntree<T>::iterator(this,0));
}

template<typename T>
typename ntree<T>::const_postfix_iterator ntree<T>::postfix_begin(void) const
{
  return ntree<T>::const_postfix_iterator(ntree<T>::const_iterator(this,m_root));
}

template<typename T>
typename ntree<T>::postfix_iterator ntree<T>::postfix_begin(void)
{
  return ntree<T>::postfix_iterator(ntree<T>::iterator(this,m_root));
}

template<typename T>
typename ntree<T>::const_postfix_iterator ntree<T>::postfix_end(void) const
{
  return ntree<T>::const_postfix_iterator(ntree<T>::const_iterator(this,0));
}

template<typename T>
typename ntree<T>::postfix_iterator ntree<T>::postfix_end(void)
{
  return ntree<T>::postfix_iterator(ntree<T>::iterator(this,0));
}

template<typename T>
typename ntree<T>::iterator ntree<T>::insert(const T& data)
{
  // insert a new node as the root
  delete m_root;
  m_root = new ntree_node<T>(data);
  return ntree<T>::iterator(this,m_root);
}

template<typename T>
typename ntree<T>::iterator ntree<T>::insert(PARAMETER_TYPENAME ntree<T>::iterator i, unsigned offset, const T& data)
  throw(wrong_object,null_dereference,end_dereference,std::out_of_range)
{
  // insert a new node as a child of i
  i.check(this);
  if (offset > children(i))
    throw std::out_of_range("ntree");
  ntree_node<T>* new_node = new ntree_node<T>(data);
  i.m_node->m_children.insert(i.m_node->m_children.begin()+offset,new_node);
  new_node->m_parent = i.m_node;
  return ntree<T>::iterator(this,new_node);
}

template<typename T>
typename ntree<T>::iterator ntree<T>::append(PARAMETER_TYPENAME ntree<T>::iterator i, const T& data)
  throw(wrong_object,null_dereference,end_dereference)
{
  return insert(i, i.m_node->m_children.size(), data);
}

template<typename T>
typename ntree<T>::iterator ntree<T>::insert(PARAMETER_TYPENAME ntree<T>::iterator i, unsigned offset, const ntree<T>& tree)
  throw(wrong_object,null_dereference,end_dereference,std::out_of_range)
{
  // insert a whole tree as a child of i
  i.check(this);
  if (offset > children(i))
    throw std::out_of_range("ntree");
  ntree_node<T>* new_node = ntree_copy(tree.m_root);
  i.m_node->m_children.insert(i.m_node->m_children.begin()+offset,new_node);
  new_node->m_parent = i.m_node;
  return ntree<T>::iterator(this,new_node);
}

template<typename T>
typename ntree<T>::iterator ntree<T>::append(PARAMETER_TYPENAME ntree<T>::iterator i, const ntree<T>& tree)
  throw(wrong_object,null_dereference,end_dereference)
{
  return insert(i, children(i), tree);
}

template<typename T>
void ntree<T>::erase(void)
{
  // erase the whole tree
  delete m_root;
  m_root = 0;
}

template<typename T>
void ntree<T>::erase(PARAMETER_TYPENAME ntree<T>::iterator i)
  throw(wrong_object,null_dereference,end_dereference)
{
  // erase this node and its subtree
  // do this by erasing this child of its parent
  // handle the case of erasing the root
  i.check(this);
  if (i.m_node == m_root)
  {
    delete m_root;
    m_root = 0;
  }
  else
  {
    DEBUG_ASSERT(i.m_node->m_parent);
    i.m_node->m_parent->m_children.erase(std::find(i.m_node->m_parent->m_children.begin(),
                                                   i.m_node->m_parent->m_children.end(),
                                                   i.m_node));
    delete i.m_node;
  }
}

template<typename T>
void ntree<T>::erase(PARAMETER_TYPENAME ntree<T>::iterator i, unsigned offset)
  throw(wrong_object,null_dereference,end_dereference,std::out_of_range)
{
  i.check(this);
  if (offset > children(i))
    throw std::out_of_range("ntree");
  delete i.m_node->m_children[offset];
  i.m_node->m_children.erase(i.m_node->m_children.begin() + offset);
}

template<typename T>
ntree<T> ntree<T>::subtree(void)
{
  ntree<T> result;
  result.m_root = ntree_copy(m_root);
  return result;
}

template<typename T>
ntree<T> ntree<T>::subtree(PARAMETER_TYPENAME ntree<T>::iterator i)
  throw(wrong_object,null_dereference,end_dereference)
{
  i.check(this);
  ntree<T> result;
  result.m_root = ntree_copy(i.m_node);
  return result;
}

template<typename T>
ntree<T> ntree<T>::subtree(PARAMETER_TYPENAME ntree<T>::iterator i, unsigned offset)
  throw(wrong_object,null_dereference,end_dereference,std::out_of_range)
{
  // copy the child to form a new subtree
  i.check(this);
  if (offset > children(i))
    throw std::out_of_range("ntree");
  ntree<T> result;
  result.m_root = ntree_copy(i.m_node->m_children[offset]);
  return result;
}

template<typename T>
ntree<T> ntree<T>::cut(void)
{
  ntree<T> result;
  result.m_root = m_root;
  m_root = 0;
  return result;
}

template<typename T>
ntree<T> ntree<T>::cut(PARAMETER_TYPENAME ntree<T>::iterator i)
  throw(wrong_object,null_dereference,end_dereference)
{
  i.check(this);
  ntree<T> result;
  if (i.m_node == m_root)
  {
    result.m_root = m_root;
    m_root = 0;
  }
  else
  {
    DEBUG_ASSERT(i.m_node->m_parent);
    typename std::vector<ntree_node<T>*>::iterator found = 
      std::find(i.m_node->m_parent->m_children.begin(), i.m_node->m_parent->m_children.end(), i.m_node);
    DEBUG_ASSERT(found != i.m_node->m_children.end());
    result.m_root = *found;
    i.m_node->m_parent->m_children.erase(found);
  }
  if (result.m_root) result.m_root->m_parent = 0;
  return result;
}

template<typename T>
ntree<T> ntree<T>::cut(PARAMETER_TYPENAME ntree<T>::iterator i, unsigned offset)
  throw(wrong_object,null_dereference,end_dereference,std::out_of_range)
{
  i.check(this);
  if (offset > children(i))
    throw std::out_of_range("ntree");
  ntree<T> result;
  result.m_root = i.m_node->m_children[offset];
  i.m_node->m_children.erase(i.m_node->m_children.begin() + offset);
  if (result.m_root) result.m_root->m_parent = 0;
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// DJDM added comparison operators

template<typename T>
bool ntree<T>::operator < (const PARAMETER_TYPENAME ntree<T>& right) const
{
  return ntree_less(m_root, right.m_root);
}

template<typename T>
bool ntree<T>::operator == (const PARAMETER_TYPENAME ntree<T>& right) const
{
  return ntree_equal(m_root, right.m_root);
}

////////////////////////////////////////////////////////////////////////////////

template<typename T>
otext& ntree<T>::print(otext& str, unsigned indent) const
{
  return ntree_print(str, m_root, indent);
}

template<typename T>
otext& print_ntree(otext& str, const ntree<T>& tree, unsigned indent)
{
  return tree.print(str, indent);
}

template<typename T>
otext& operator << (otext& str, const ntree<T>& tree)
{
  return print_ntree(str, tree, 0);
}

////////////////////////////////////////////////////////////////////////////////
// persistence

template<typename T>
void ntree<T>::dump_r(dump_context& context, PARAMETER_TYPENAME ntree<T>::const_iterator node) const throw(persistent_dump_failed)
{
  // the address of the node is dumped as well as the contents - this is used in iterator persistence
  std::pair<bool,unsigned> node_mapping = context.pointer_map(node.m_node);
  if (node_mapping.first) throw persistent_dump_failed("ntree: already dumped this node");
  ::dump(context,node_mapping.second);
  // now dump the contents
  ::dump(context,*node);
  /// dump the number of children
  ::dump(context,children(node));
  // recurse on the children
  for (unsigned i = 0; i < children(node); i++)
    dump_r(context,child(node,i));
}

template<typename T>
void ntree<T>::dump(dump_context& context) const
  throw(persistent_dump_failed)
{
  // dump a magic key to the address of the tree for use in persistence of iterators
  // and register it as a dumped address
  std::pair<bool,unsigned> mapping = context.pointer_map(this);
  if (mapping.first) throw persistent_dump_failed("ntree: already dumped this tree");
  ::dump(context,mapping.second);
  // now dump the tree contents
  ::dump(context, empty());
  if (!empty())
    dump_r(context,root());
}

template<typename T>
void ntree<T>::restore_r(restore_context& context, PARAMETER_TYPENAME ntree<T>::iterator node) throw(persistent_restore_failed)
{
  // restore the node magic key, check whether it has been used before and add it to the set of known addresses
  unsigned node_magic = 0;
  ::restore(context,node_magic);
  std::pair<bool,void*> node_mapping = context.pointer_map(node_magic);
  if (node_mapping.first)
    throw persistent_restore_failed("ntree: restored this tree node already");
  context.pointer_add(node_magic,node.m_node);
  // now restore the node contents
  ::restore(context,*node);
  // restore the number of children
  unsigned children = 0;
  ::restore(context,children);
  // recurse on each child
  for (unsigned i = 0; i < children; i++)
  {
    ntree<T>::iterator child = insert(node,i,T());
    restore_r(context,child);
  }
}

template<typename T>
void ntree<T>::restore(restore_context& context)
  throw(persistent_restore_failed)
{
  erase();
  // restore the tree's magic key and map it onto the tree's address
  // this is used in the persistence of iterators
  unsigned magic = 0;
  ::restore(context,magic);
  context.pointer_add(magic,this);
  // now restore the contents
  bool empty = true;
  ::restore(context, empty);
  if (!empty)
  {
    ntree<T>::iterator node = insert(T());
    restore_r(context,node);
  }
}

template<typename T>
void dump_ntree(dump_context& context, const ntree<T>& data) throw(persistent_dump_failed)
{
  data.dump(context);
}

template<typename T>
void restore_ntree(restore_context& context, ntree<T>& data) throw(persistent_restore_failed)
{
  data.restore(context);
}

////////////////////////////////////////////////////////////////////////////////
