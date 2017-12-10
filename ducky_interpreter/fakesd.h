#define Fakesd_d


class Fakesd{
  private:
    char scriptraw[400];
    unsigned long read_position;
  public:
    Fakesd();
    char read();
    bool available();
    unsigned long position();
    bool seek(unsigned long new_position);
    void close();
};
