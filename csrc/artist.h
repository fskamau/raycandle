
struct ArtistVTable{
  void (*init)(Artist* artist,Axes* axes);
  Limit (*get_limit)(Artist* artist, Axes* axes);
  void (*draw)(Artist* artist, Axes* axes, Rect area);
  void (*draw_icon)(Artist* artist, Axes* axes, Vector2 start_pos);
};
