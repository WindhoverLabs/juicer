#define USE_VIRTUAL
#define USE_ARRAY
struct t1
{
    typedef struct
    {
    } t2;

#ifdef USE_VIRTUAL
    virtual
#endif

        void
        f1();
};

t1::t2 v1
#ifdef USE_ARRAY
    [1]
#endif
    ;