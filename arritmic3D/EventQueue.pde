

class Evento {
  int id;
  float t;
  int st;
  int pos; // position in the array inside the queue

  Evento() {
    id = -1;
    t = 0.0;
    st = -1;
    pos = -1;
  }
}

int sign(float x) { return x > 0.0 ? 1 : -1;}

class OrdenaEventos implements Comparator<Evento> {
  public int compare(Evento one, Evento another) {
    return sign(one.t - another.t);
  }
}

class EventQueue{

  // Attributes
  ArrayList<Evento> tree;
  OrdenaEventos ord;

  // public methods
  EventQueue() {
    tree = new ArrayList<Evento>();
    ord = new OrdenaEventos();
  }

  void clear() {
    tree.clear();
  }

  boolean isEmpty() {
    return size() == 0;
  }

  int size() {
    return tree.size();
  }

  void add(Evento ev) {
    if(ev.pos != -1) {
      if(!update(ev))
        insert(ev);
    }
    else
      insert(ev);
  }

  void insert(Evento ev) {
    tree.add(ev);
    ev.pos = lasti();
    moveUp(lasti());
  }

  boolean update(Evento ev){
    int i = ev.pos;
    boolean updated = false;

    if(ev != tree.get(i)) {
      println(" WARNING-QUEUE: the event has a position, but it is occupied by another event. Inserting it.");
    } else {
      if( i > 0 && lessThan(i, predec(i)) )
        moveUp(i);
      else if( ( inRange(leftDesc(i)) && lessThan(leftDesc(i),i) ) ||
                ( inRange(rightDesc(i)) && lessThan(rightDesc(i),i) ) )
        moveDown(i);
      updated = true;
    }
    return updated;
  }

  Evento updateTime(int pos, float t) {
    Evento ev = null;
    if( inRange(pos) ) {
      ev = tree.get(pos);
      float old_t = ev.t;
      if(old_t < t) {
        ev.t = t;
        moveDown(pos);
      } else if(ev.t > t) {
        ev.t = t;
        moveUp(pos);
      }
    }
    return ev;
  }

  Evento poll() {
    Evento first = null;
    if(!isEmpty()) {
      first = peek();
      tree.set(0,tree.get(lasti()));
      tree.remove(lasti());
      moveDown(0);
      first.pos = -1;
    }
    return first;
  }

  Evento peek() {
    Evento first = null;
    if(!isEmpty())
      first = tree.get(0);
    return first;
  }

  void print() {
    for (Evento e : tree ) {
      println(e.id, e.t, e.st, e.pos);
    }
    println("  +");
  }

  // Private methods

  private int leftDesc(int i) {
    return i*2 + 1;
  }

  private int rightDesc(int i) {
    return i*2 + 2;
  }

  private int predec(int i) {
    return (i-1)/2;
  }

  private void moveUp(int i) {
    while( i > 0 && lessThan( i, predec(i) ) ) {
      swap(i,predec(i));
      i = predec(i);
    }
  }

  private void moveDown(int i) {
    int min = i;

    do {
      i = min;
      int ld = leftDesc(i);
      if( inRange(ld) && lessThan(ld,min))
        min = ld;

      int rd = rightDesc(i);
      if(inRange(rd) && lessThan(rd,min))
        min = rd;

      if( min != i)
        swap(min,i);

    } while( min != i );
  }

  private void swap(int i, int j) {
    Evento old_i = tree.get(i);
    tree.set(i,tree.get(j));
    tree.get(i).pos = i;

    tree.set(j,old_i);
    old_i.pos = j;

  }

  private int lasti() {
    return tree.size() - 1;
  }

  private boolean lessThan(int i,int j) {
    Evento ei = tree.get(i);
    Evento ej = tree.get(j);
    return ord.compare(ei,ej) < 0;
  }

  private boolean inRange(int i) {
    return i>=0 && i < tree.size();
  }

}
