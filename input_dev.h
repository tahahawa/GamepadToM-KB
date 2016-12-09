class input_dev {
private:
  struct libevdev *dev;
  int fd;
  bool validInput(struct input_event *);
  int deadzone[4][2];
  const gchar *fname;
  gchar *gstr;
  GDir *dir;
  int rc;

public:
  input_dev();
  ~input_dev();

  void poll(std::vector<struct input_event> *events);
};
