#ifndef CCHANNEL_HPP_
#define CCHANNEL_HPP_

class CChannel
{
   public:
    typedef enum
    {
        UNUSED   = 0,
        INACTIVE = 1,
        ACTIVE   = 2,
    } TState;

    typedef struct
    {
        TState State;
        int    data;
    } SHK;

    CChannel();
    virtual void getData();
    ~CChannel();

   private:
    SHK Hk;
};

#endif /* CCHANNEL_HPP_ */