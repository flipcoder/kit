#ifndef MATRIXSTACK_H
#define MATRIXSTACK_H

//#include "matrix.h"
#include "common.h"

// Example:
// matrixstack.push()->translate(Vector3(1.0, 2.0, 3.0f);
// to grab matrix at any time, use *matrixstack.top()
// matrixstack.pop();

class MatrixStack
{
public:
    struct Node {
        Node(glm::mat4* _data = NULL, Node* _next = NULL) {
            data = _data;
            next = _next;
        }
        ~Node(){
            delete data;
            delete next;
        }
        glm::mat4* data;
        Node* next;
    };

    class ScopedPop
    {
    public:
        explicit ScopedPop(MatrixStack& ms):
            m_Stack(&ms)
        {}
        ~ScopedPop() {
            if(m_Stack)
                m_Stack->pop();
        }
    private:
        MatrixStack* m_Stack;
    };
private:
    Node* m_pHead;
public:
    MatrixStack();
    virtual ~MatrixStack();
    void clear();
    void identity();
    glm::mat4* push();
    void push(const glm::mat4& m);
    void push_post(const glm::mat4& m);
    void push_inverse(const glm::mat4& m);
    void push_inverse_post(const glm::mat4& m);
    bool pop();
    bool pop(glm::mat4& m);
    unsigned int size() const;
    bool empty() { return m_pHead!=NULL; }
    const glm::mat4* top_c() const {
        if(!m_pHead)
            return NULL;
        return m_pHead->data;
    }
    glm::mat4* top() {
        if(!m_pHead)
            return NULL;
        return m_pHead->data;
    }
    //glm::mat4 calc_inverse() {
    //    if(m_pHead)
    //        return glm::inverse(*top());
    //    else
    //        return glm::mat4();
    //}
    //glm::mat4 calc_inverse2() {
    //    if(m_pHead)
    //    {
    //        glm::mat4 m = glm::inverse(*top());
    //        Vector3 t = top()->translation();
    //        m.setTranslation(-t);
    //        return m;
    //    }
    //    else
    //        return glm::mat4(1.0f);
    //}
};

#endif

