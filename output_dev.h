class output_dev {
private:
  struct uinput_user_dev uindev;
  int fd;
  void setupAllowedEvents(int *);

public:
  output_dev();
  ~output_dev();
  void send(std::vector<struct input_event>, bool);
};
