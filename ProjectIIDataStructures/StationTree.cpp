#include "StationTree.h"
#include <utility>

StationTree::Node::Node(const Station &station) : data(station), left(nullptr), right(nullptr)
{
}

StationTree::StationTree() : root(nullptr), nodeCount(0)
{
}

StationTree::~StationTree()
{
    clear();
}

bool StationTree::insert(const Station &station)
{
    if (insert(root, station))
    {
        nodeCount++;
        return true;
    }
    return false;
}

bool StationTree::remove(int id)
{
    if (remove(root, id))
    {
        nodeCount--;
        return true;
    }
    return false;
}

bool StationTree::find(int id, Station &station) const
{
    return find(root, id, station);
}

std::vector<Station> StationTree::inOrder() const
{
    std::vector<Station> result;
    inOrder(root, result);
    return result;
}

std::vector<Station> StationTree::preOrder() const
{
    std::vector<Station> result;
    preOrder(root, result);
    return result;
}

std::vector<Station> StationTree::postOrder() const
{
    std::vector<Station> result;
    postOrder(root, result);
    return result;
}

void StationTree::clear()
{
    clear(root);
    root = nullptr;
    nodeCount = 0;
}

bool StationTree::isEmpty() const
{
    return nodeCount == 0;
}

int StationTree::size() const
{
    return nodeCount;
}

void StationTree::forEach(const std::function<void(Station &)> &callback)
{
    forEach(root, callback);
}

bool StationTree::insert(Node *&node, const Station &station)
{
    if (!node)
    {
        node = new Node(station);
        return true;
    }
    if (station.getId() < node->data.getId())
    {
        return insert(node->left, station);
    }
    if (station.getId() > node->data.getId())
    {
        return insert(node->right, station);
    }
    return false;
}

bool StationTree::remove(Node *&node, int id)
{
    if (!node)
    {
        return false;
    }
    if (id < node->data.getId())
    {
        return remove(node->left, id);
    }
    if (id > node->data.getId())
    {
        return remove(node->right, id);
    }
    if (!node->left && !node->right)
    {
        delete node;
        node = nullptr;
        return true;
    }
    if (!node->left)
    {
        Node *temp = node;
        node = node->right;
        delete temp;
        return true;
    }
    if (!node->right)
    {
        Node *temp = node;
        node = node->left;
        delete temp;
        return true;
    }
    Node *minNode = findMin(node->right);
    node->data = minNode->data;
    return remove(node->right, minNode->data.getId());
}

StationTree::Node *StationTree::findMin(Node *node)
{
    Node *current = node;
    while (current && current->left)
    {
        current = current->left;
    }
    return current;
}

bool StationTree::find(Node *node, int id, Station &station) const
{
    if (!node)
    {
        return false;
    }
    if (id < node->data.getId())
    {
        return find(node->left, id, station);
    }
    if (id > node->data.getId())
    {
        return find(node->right, id, station);
    }
    station = node->data;
    return true;
}

void StationTree::inOrder(Node *node, std::vector<Station> &result) const
{
    if (!node)
    {
        return;
    }
    inOrder(node->left, result);
    result.push_back(node->data);
    inOrder(node->right, result);
}

void StationTree::preOrder(Node *node, std::vector<Station> &result) const
{
    if (!node)
    {
        return;
    }
    result.push_back(node->data);
    preOrder(node->left, result);
    preOrder(node->right, result);
}

void StationTree::postOrder(Node *node, std::vector<Station> &result) const
{
    if (!node)
    {
        return;
    }
    postOrder(node->left, result);
    postOrder(node->right, result);
    result.push_back(node->data);
}

void StationTree::clear(Node *node)
{
    if (!node)
    {
        return;
    }
    clear(node->left);
    clear(node->right);
    delete node;
}

void StationTree::forEach(Node *node, const std::function<void(Station &)> &callback)
{
    if (!node)
    {
        return;
    }
    callback(node->data);
    forEach(node->left, callback);
    forEach(node->right, callback);
}
