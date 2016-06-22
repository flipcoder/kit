#include "matrixstack.h"
#include "../kit.h"
#include "common.h"
#include <cassert>

MatrixStack :: MatrixStack()
{
    //m_pHead = NULL;
    m_pHead = new Node(new glm::mat4(), NULL);
}

MatrixStack :: ~MatrixStack()
{
    delete m_pHead;
}

void MatrixStack :: identity()
{
    delete m_pHead;
    m_pHead = new Node(new glm::mat4(), NULL);
}

void MatrixStack :: clear()
{
    delete m_pHead;
    m_pHead = NULL;
}

glm::mat4* MatrixStack :: push()
{
    glm::mat4* new_matrix;
    if(!m_pHead)
        identity();
    //if(!m_pHead)
    //    new_matrix = new Matrix(Matrix::IDENTITY);
    //else
    new_matrix = new glm::mat4(*m_pHead->data);

    m_pHead = new Node(new_matrix, m_pHead);
    return m_pHead->data;
}

void MatrixStack :: push_post(const glm::mat4& m)
{
    glm::mat4* new_matrix;
    if(!m_pHead)
        identity();

    new_matrix = new glm::mat4(*m_pHead->data);
    *new_matrix *= m;

    m_pHead = new Node(new_matrix, m_pHead);
}

void MatrixStack :: push(const glm::mat4& m)
{
    glm::mat4* new_matrix;
    if(!m_pHead)
        identity();

    new_matrix = new glm::mat4(*m_pHead->data);
    *new_matrix = m * *new_matrix;

    m_pHead = new Node(new_matrix, m_pHead);
}


void MatrixStack :: push_inverse(const glm::mat4& m)
{
    glm::mat4* new_matrix;
    if(!m_pHead)
        identity();
        //new_matrix = new Matrix(Matrix::IDENTITY);

    new_matrix = new glm::mat4(*m_pHead->data);
    *new_matrix = glm::inverse(m) * *new_matrix;
    //*new_matrix *= glm::inverse(m);

    m_pHead = new Node(new_matrix, m_pHead);
}

void MatrixStack :: push_inverse_post(const glm::mat4& m)
{
    glm::mat4* new_matrix;
    if(!m_pHead)
        identity();

    new_matrix = new glm::mat4(*m_pHead->data);
    //*new_matrix *= Matrix(Matrix::INVERSE, m);
    //*new_matrix = glm::inverse(m) * *new_matrix;
    *new_matrix *= glm::inverse(m);

    m_pHead = new Node(new_matrix, m_pHead);
}

bool MatrixStack :: pop()
{
    assert(m_pHead);
    if(!m_pHead)
        return false;

    Node* delete_me = m_pHead;
    Node* new_head = m_pHead->next;
    delete_me->next = NULL; // preserve
    m_pHead = new_head;
    delete delete_me;

    return true;
}

bool MatrixStack :: pop(glm::mat4& m)
{
    assert(m_pHead);
    if(!m_pHead)
        return false;

    Node* delete_me = m_pHead;
    Node* new_head = m_pHead->next;
    delete_me->next = NULL; // preserve
    m_pHead = new_head;
    m = *delete_me->data; // grab popped data
    delete delete_me;

    return true;
}

unsigned int MatrixStack :: size() const
{
    unsigned int count = 0;
    const Node* n = m_pHead;
    while(n)
    {
        ++count;
        n = n->next;
    }
    return count;
}

