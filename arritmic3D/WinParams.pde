
class Params extends PApplet {


  Params() {
    super();
    PApplet.runSketch(new String[] {this.getClass().getSimpleName()}, this);
  }

  void settings() {
    size(500, 400);
  }

  void setup() {

    PFont font = createFont("Ubuntu",16);
    frameRate(caseParams.fps);

  }

  void draw() {
     background(225);
     noStroke();
     fill(0);
     draw_params(ac.n_cells_updated,t_fin_ciclo,ac_first_start_time, ac_LastStimTime, ac_first_lpath );
  }

  void mousePressed() {
   // println("mousePressed in secondary window");
  }


void draw_params(int n_cells_updated, float t1, float ac_first_start_time, float ac_LastStimTime, float ac_first_lpath){

  int margen = 35;
  int altura = 120;

  if (ac == null)
    return;

  stroke(0,0,0);
  fill(0,0,0);

  textSize(20);
  text("T: "+str(tiempo_transcurrido)+"\ndt: "+str(caseParams.dt)+"\nn; "+n_cells_updated, margen,30);

  textSize(14);

  altura+=25;
  textSize(14);
  text("'m': change the vis. mode  (life_time vs cell_state)", margen, altura,0);
  altura+=25;
  text("Visualization mode: "+str(caseParams.vmode), margen, altura,0);
  altura+=25;
  text("Beat: "+str(ac.num_beat + 1), margen,altura,0);
  altura+=25;
  textSize(16);
  //PVector err = ac.get_errors();
  if (ac.first != null){

    text("Longest Path: "+str(ac_first_lpath)+ " mm", margen, altura,0);
    altura+=25;
    text("LP-Time: "+str(ac_LastStimTime)+ " msec", margen, altura,0);
    altura+=25;
    if ((ac_first_start_time - ac_LastStimTime) != 0)
      text("LP-CV: "+str(ac_first_lpath/(ac_first_start_time - ac_LastStimTime))+ " msec", margen, altura,0);
    //text("LP-distance error: "+str(ac.first.derror), margen, 105,0);
    //text("LP-temporal error: "+str(ac.first.terror) , margen, 130,0);

    //text("AbsDist-Error (up): "+str(err.x)+"\nAbsTime-Error (down): "+str(err.y), -350,165,0);
    //text("AvgDistError: "+str(err.x/ac.G.size())+"\nAvgTimeError: "+str(err.y/ac.G.size()), -350,240,0);
  }
  altura+=25;
  text("Comp. time = "+t1+"ms",margen,altura);

}


}
