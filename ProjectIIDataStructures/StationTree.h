#pragma once

#include "Station.h"
#include <vector>

class StationTree
{
public:
    StationTree();
    ~StationTree();
    bool insert(const Station &station);
    bool remove(int id);
    bool find(int id, Station &station) const;
    std::vector<Station> inOrder() const;
    std::vector<Station> preOrder() const;
    std::vector<Station> postOrder() const;
    void clear();
    bool isEmpty() const;
    int size() const;
private:
    struct Node
    {
        Station data;
        Node *left;
        Node *right;
        Node(const Station &station);
    };
    Node *root;
    int nodeCount;
    bool insert(Node *&node, const Station &station);
    bool remove(Node *&node, int id);
    Node *findMin(Node *node);
    bool find(Node *node, int id, Station &station) const;
    void inOrder(Node *node, std::vector<Station> &result) const;
    void preOrder(Node *node, std::vector<Station> &result) const;
    void postOrder(Node *node, std::vector<Station> &result) const;
    void clear(Node *node);
};
