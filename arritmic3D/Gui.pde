// Añadir el label del caso y el check de multiview


import controlP5.*;

ControlP5 gui;

// Botones
Boolean gui_play               = false;
Boolean gui_pause              = true;
Boolean gui_init               = false;
int     gui_activation_mode    = 0;     // Frente activación -> 1, Punto activación inicial -> 0 (En BLOQUE_VTK coge el id de nodo inicial de archivo params id_extraI)
                                        // Bloque activacion -> 2 // giada

// Input -> valores
float   gui_stimFrec_hz        = 0; // Si 1 Frente activación inicial Horiz de Izq a Dcha
float   gui_stimFrec_vt        = 1; // Si 1 Frente activación inicial Vert de Abajo a Arriba
//int     gui_xfoco              = 0;
//int     gui_yfoco              = 0;
float   gui_vdelay             = 0;
float   gui_hdelay             = 0;
float   gui_pdelay             = 0;
int     gui_id_start_cell      ;
/*
// DATOS BLOQUE6
int     gui_blk_xmin           = 0;
int     gui_blk_xmax           = 250;
int     gui_blk_ymin           = 0;
int     gui_blk_ymax           = 250;
*/

// DATOS PARCHE BLOQUE_VTK
//int     gui_blk_id_minXY       = 81;   // ID de la celda (ELEMENTO) inicial con el mínimo x e y, del bloque de activación
int     gui_blk_idNodo_minXY   = 47075; // ID del punto (NODO) inicial con el mínimo x e y, del parche de activación //DOLORS: 67536 //giada dicembre22: 27562
int     gui_blk_num_xmax       = 133; // Numero de celdas en x del parche de activación (86)
int     gui_blk_num_ymax       = 132;   // Numero de celdas en y del parche de activación (76)

float   gui_blk_delay          = 0;
int     gui_blk_activated      = 0; // Bloque activado -> 1, desactivado -> 0

Boolean gui_save               = false;
Boolean gui_load               = false;

String  gui_current_state_file = "";




public class InputTest {
  String path;

  InputTest() {
    selectInput("Select a file to process:", "fileSelected", dataFile(sketchPath()), this);
  }

  void fileSelected(File selection) {
    if (selection == null)
      println("Window was closed or the user hit cancel.");

    else if (!selection.isFile())
      println("\"" + selection + "\" is an invalid file.");

    else{
        println("Loading state " + (gui_current_state_file = selection.getAbsolutePath()));
        ac.load_state(new File(gui_current_state_file));
    }
}
}

class Controles extends PApplet {

  RadioButton r1;
  CheckBox chk_blk, chk_cmap,chk_hay_mesh, use_restsurf;
  // CheckBox chk_show_path, chk_show_grid;
  PImage imgs;
  color activeC = #1673e5;


  Controles() {
    super();
    PApplet.runSketch(new String[] {this.getClass().getSimpleName()}, this);
  }

  void settings() {
    size(500, 600);
  }

  void setup() {

    PFont font = createFont("Ubuntu",13);
    //imgs = loadImage("/tmp/play.png");
    int tam_butt_x = 45;
    int tam_butt_y = 28;

    gui = new ControlP5(this);
    gui.setFont(font);

    gui_id_start_cell = caseParams.id_extraI;

    // Input data
    gui.addTextfield("frec_s1")
     .setPosition(20,25)
     .setSize(70,30)
     .setFont(font)
     .setValue(str(stimFrecS1))
     .setFocus(true)
     .setColorForeground(color(255))
     .setColorActive(color(255))
     .setColorLabel(color(0))
     .setAutoClear(false)
     ;

    gui.addTextfield("frec_s2")
     .setPosition(20,80)
     .setSize(70,30)
     .setFont(font)
     .setValue(str(stimFrecS2))
     .setFocus(true)
     .setColorForeground(color(255))
     .setColorActive(color(255))
     .setColorLabel(color(0))
     .setAutoClear(false)
     ;

    gui.addTextfield("num_s1")
     .setPosition(120,25)
     .setSize(40,30)
     .setFont(font)
     .setValue(str(nStimsS1))
     .setFocus(true)
     .setColorForeground(color(255))
     .setColorActive(color(255))
     .setColorLabel(color(0))
     .setAutoClear(false)
     ;

    gui.addTextfield("num_s2")
     .setPosition(120,80)
     .setSize(40,30)
     .setFont(font)
     .setValue(str(nStimsS2))
     .setFocus(true)
     .setColorForeground(color(255))
     .setColorActive(color(255))
     .setColorLabel(color(0))
     .setAutoClear(false)
     ;

    int altG = 350;
    if (caso == VENT)
      altG = 250;


    chk_cmap = gui.addCheckBox("color_map")
       .setPosition(20,altG)
       .setSize(15,15)
       .setColorForeground(activeC)
       .setColorBackground(color(255))
       .setColorActive(activeC)
       .setColorLabel(color(0))
       .setItemsPerRow(1)
       .addItem("Hide color map",1)
       ;
     if (caseParams.show_cmap)
       chk_cmap.activate(1);
     else
      chk_cmap.activate(0);

    chk_hay_mesh = gui.addCheckBox("show_mesh")
       .setPosition(20,altG+20)
       .setSize(15,15)
       .setColorForeground(activeC)
       .setColorBackground(color(255))
       .setColorActive(activeC)
       .setColorLabel(color(0))
       .setItemsPerRow(1)
       .addItem("Hide mesh",1)
       ;

    if (caseParams.hay_mesh)
       chk_hay_mesh.activate(1);
    else
       chk_hay_mesh.activate(0);

   use_restsurf = gui.addCheckBox("use_restsurface")
       .setPosition(20,altG+40)
       .setSize(15,15)
       .setColorForeground(activeC)
       .setColorBackground(color(255))
       .setColorActive(activeC)
       .setColorLabel(color(0))
       .setItemsPerRow(1)
       .addItem("Use Restitution Surface",1)
       ;
    if (caseParams.restsurf)
       use_restsurf.activate(1);
    else
       use_restsurf.activate(0);

   float dt_ini = caseParams.dt;
   gui.addSlider("dt")
     .setPosition(270,altG)
     .setSize(130,20)
     .setRange(0.1,25)
     .setValue(dt_ini)
     .setColorValue(color(0))
     .setColorLabel(color(0))
     ;

   // reposition the Label for controller 'slider'
   gui.getController("dt").getValueLabel().align(ControlP5.LEFT, ControlP5.BOTTOM_OUTSIDE).setPaddingX(0);
   gui.getController("dt").getCaptionLabel().align(ControlP5.RIGHT, ControlP5.BOTTOM_OUTSIDE).setPaddingX(0);



    /*
    chk_show_path = gui.addCheckBox("show_path")
       .setPosition(20,330)
       .setSize(15,15)
       .setColorForeground(color(120))
       .setColorActive(color(255))
       .setColorLabel(color(0))
       .setItemsPerRow(1)
       .addItem("Show path",1)
       ;

    chk_show_grid = gui.addCheckBox("show_grid")
       .setPosition(20,350)
       .setSize(15,15)
       .setColorForeground(color(120))
       .setColorActive(color(255))
       .setColorLabel(color(0))
       .setItemsPerRow(1)
       .addItem("Show grid",1)
       ;
    */

   if (caso == BLOQUE_VTK){

     gui.addTextfield("h_delay")
       .setPosition(285,55)
       .setSize(tam_butt_x,tam_butt_y)
       .setFont(font)
       .setValue(str(gui_hdelay))
       .setColorForeground(color(255))
       .setColorActive(color(255))
       .setColorLabel(color(0))
       .setAutoClear(false)
       ;

       gui.addTextfield("frec_horiz")
       .setPosition(385,55)
       .setSize(tam_butt_x,tam_butt_y)
       .setFont(font)
       .setValue(str(gui_stimFrec_hz))
       .setColorForeground(color(255))
       .setColorActive(color(255))
       .setColorLabel(color(0))
       .setAutoClear(false)
       ;

       gui.addTextfield("v_delay")
       .setPosition(285,110)
       .setSize(tam_butt_x,tam_butt_y)
       .setFont(font)
       .setValue(str(gui_vdelay))
       .setColorForeground(color(255))
       .setColorActive(color(255))
       .setColorLabel(color(0))
       .setAutoClear(false)
       ;

       gui.addTextfield("frec_vert")
       .setPosition(385,110)
       .setSize(tam_butt_x,tam_butt_y)
       .setFont(font)
       .setValue(str(gui_stimFrec_vt))
       .setColorForeground(color(255))
       .setColorActive(color(255))
       .setColorLabel(color(0))
       .setAutoClear(false)
       ;

        gui.addTextfield("id_start_node")
       .setPosition(285,210)
       .setSize(int(tam_butt_x*1.5),tam_butt_y)
       .setFont(font)
       .setValue(str(caseParams.id_extraI))
       .setColorForeground(color(255))
       .setColorActive(color(255))
       .setColorLabel(color(0))
       .setAutoClear(false)
       ;

       /*
       gui.addTextfield("y")
       .setPosition(385,210)
       .setSize(tam_butt_x,tam_butt_y)
       .setFont(font)
       .setValue(str(gui_yfoco))
       .setColorForeground(color(255))
       .setColorActive(color(255))
       .setColorLabel(color(0))
       .setAutoClear(false)
       ;
       */

        gui.addTextfield("p_delay")
       .setPosition(285,265)
       .setSize(tam_butt_x,tam_butt_y)
       .setFont(font)
       .setValue(str(0.0))
       .setColorForeground(color(255))
       .setColorActive(color(255))
       .setColorLabel(color(0))
       .setAutoClear(false)
       ;

       gui.addTextfield("Min_ID_node_X")
       .setPosition(30,210)
       .setSize(tam_butt_x,tam_butt_y)
       .setFont(font)
       .setValue(str(gui_blk_idNodo_minXY))
       .setColorForeground(color(255))
       .setColorActive(color(255))
       .setColorLabel(color(0))
       .setAutoClear(false)
       ;

        gui.addTextfield("Num_node_X")
       .setPosition(30,265)
       .setSize(tam_butt_x,tam_butt_y)
       .setFont(font)
       .setValue(str(gui_blk_num_xmax))
       .setColorForeground(color(255))
       .setColorActive(color(255))
       .setColorLabel(color(0))
       .setAutoClear(false)
       ;

       gui.addTextfield("Num_node_Y")
       .setPosition(130,265)
       .setSize(tam_butt_x,tam_butt_y)
       .setFont(font)
       .setValue(str(gui_blk_num_ymax))
       .setColorForeground(color(255))
       .setColorActive(color(255))
       .setColorLabel(color(0))
       .setAutoClear(false)
       ;

       // RadioButtons : Activación frente vs punto
       // giada: anyado tambien el bloque

       r1 = gui.addRadioButton("radioButton")
           .setPosition(260,30)
           .setSize(15,15)
           .setColorForeground(activeC)
           .setColorBackground(color(255))
           .setColorActive(activeC)
           .setColorLabel(color(0))
           .setItemsPerRow(1)
           .setSpacingRow(140)
           .setNoneSelectedAllowed(false)
           //.setNoneSelectedAllowed(true) //GIADA
           .addItem("Activar Frente",1)
           .addItem("Activar Punto",0)
           //.addItem("Activar Bloque",2) //ADD GIADA 02/12/22
           .activate(1)
           ;

      chk_blk = gui.addCheckBox("checkBox")
           .setPosition(20,185)
           .setSize(15,15)
           .setColorForeground(activeC)
           .setColorBackground(color(255))
           .setColorActive(activeC)
           .setColorLabel(color(0))
           .setItemsPerRow(1)
           .addItem("Activar Bloque",0)
           //.activate(1) //ADD GIADA
           ;


   }   //  Fin Ctroles Bloque_vtk
   else
   if (caso == VENT){

     r1 = gui.addRadioButton("radioButton")
             .setPosition(260,30)
             .setSize(15,15)
             .setColorForeground(activeC)
             .setColorBackground(color(255))
             .setColorActive(activeC)
             .setColorLabel(color(0))
             .addItem("multi_view",1)
             ;

      gui.addTextfield("id_start_node")
       .setPosition(260,80)
       .setSize(tam_butt_x*2,tam_butt_y)
       .setFont(font)
       .setValue(str(caseParams.id_extraI))
       .setColorForeground(color(255))
       .setColorActive(color(255))
       .setColorLabel(color(0))
       .setAutoClear(false)
       ;

             chk_blk = gui.addCheckBox("checkBox")
           .setPosition(20,185)
           .setSize(15,15)
           .setColorForeground(activeC)
           .setColorBackground(color(255))
           .setColorActive(activeC)
           .setColorLabel(color(0))
           .setItemsPerRow(1)
           .addItem("Activar Bloque",0)
           //.activate(1) //ADD GIADA
           ;
  }

   PFont fontInst = createFont("Ubuntu",15);
   gui.addTextlabel("Inst1")
      .setText("- Press Enter key after editing values in text fields \n\n- Press Init button to start simulation \n\n- Press Pause / Play buttons to pause and restart simulation \n\n- Edited values only apply if multiSim param to False")
      .setPosition(20,400)
      .setColorValue(0)
      .setFont(fontInst)
      ;


   int altura_player = 540;
   int offset_x_player = 110;

   gui.addButton("play")
     .setBroadcast(false)
     .setValue(128)
     .setPosition(10+(3*offset_x_player),altura_player)
     .setSize(100,40)
     .activateBy(ControlP5.RELEASE)
     //.setImage(imgs)
     .updateSize()
     .setBroadcast(true)
     ;

   gui.addButton("pause")
     .setBroadcast(false)
     .setValue(128)
     .setSize(100,40)
     .setPosition(10+(2*offset_x_player),altura_player)
     .activateBy(ControlP5.RELEASE)
     //.setImage(imgs)
     .updateSize()
     .setBroadcast(true)
     ;

  gui.addButton("init")
     .setBroadcast(false)
     .setValue(128)
     .setSize(110,50)
     .setPosition(70,altura_player-8)
     .setColorBackground(activeC)
     .activateBy(ControlP5.RELEASE)
     //.setImage(imgs)
     .updateSize()
     .setBroadcast(true)
     ;
  /*
  gui.addButton("save")
     .setBroadcast(false)
     .setValue(128)
     .setPosition(15,altura_player - 40)
     .setSize(80,30)
     .activateBy(ControlP5.RELEASE)
     //.setImage(imgs)
     .updateSize()
     .setBroadcast(true)
     ;

   gui.addButton("load")
     .setBroadcast(false)
     .setValue(128)
     .setPosition(110,altura_player - 40)
     .setSize(80,30)
     .activateBy(ControlP5.RELEASE)
     //.setImage(imgs)
     .updateSize()
     .setBroadcast(true)
     ;

    gui.addTextfield("draw_each_n")
     .setPosition(270,390)
     .setSize(25,30)
     .setFont(font)
     .setValue(str(caseParams.pinta_cada_n))
     .setColorForeground(color(255))
     .setColorActive(color(255))
     .setColorLabel(color(0))
     .setAutoClear(false)
     ;
     */

  }

  void draw() {
     background(225);
     noStroke();
     fill(0);
     //ellipse(random(width), random(height), random(50), random(50));
  }

  void mousePressed() {
   // println("mousePressed in secondary window");
  }

  ///////////////////////////////////////////////////////////////////

  //   CB

  ////////////////////////////////////////////////////////////////////

  void controlEvent(ControlEvent theEvent) {
    if(theEvent.isAssignableFrom(Textfield.class)) {
      println("controlEvent: accessing a string from controller '"
              +theEvent.getName()+"': "
              +theEvent.getStringValue()
              );

              if (theEvent.getName() == "frec_s1") {
                stimFrecS1 = float(theEvent.getStringValue());
                ac.NextStimTime = stimFrecS1;
              }
              if (theEvent.getName() == "frec_s2")
                stimFrecS2 = float(theEvent.getStringValue());

              if (theEvent.getName() == "num_s1")
                nStimsS1 = int(theEvent.getStringValue());

              if (theEvent.getName() == "num_s2")
                nStimsS2 = int(theEvent.getStringValue());

              /*
              if (theEvent.getName() == "draw_each_n")
                caseParams.pinta_cada_n = int(theEvent.getStringValue());
              */

              if (theEvent.getName() == "id_start_node" )
                gui_id_start_cell = int(theEvent.getStringValue());

              if (caso == BLOQUE_VTK){
                    if (theEvent.getName() == "frec_horiz" )
                        gui_stimFrec_hz = float(theEvent.getStringValue());
                    if (theEvent.getName() == "frec_vert" )
                        gui_stimFrec_vt = float(theEvent.getStringValue());
                    //if (theEvent.getName() == "y" )
                    //    gui_yfoco = int(theEvent.getStringValue());

                    if (theEvent.getName() == "v_delay" )
                        gui_vdelay = float(theEvent.getStringValue());
                    if (theEvent.getName() == "h_delay" )
                        gui_hdelay = float(theEvent.getStringValue());
                    if (theEvent.getName() == "p_delay" )
                        gui_pdelay = float(theEvent.getStringValue());
                    /*
                    if (theEvent.getName() == "xmax" )
                        gui_blk_xmax = int(theEvent.getStringValue());
                    if (theEvent.getName() == "ymax" )
                        gui_blk_ymax = int(theEvent.getStringValue());
                     if (theEvent.getName() == "xmin" )
                        gui_blk_xmin = int(theEvent.getStringValue());
                    if (theEvent.getName() == "ymin" )
                        gui_blk_ymin = int(theEvent.getStringValue());
                    */
                    if (theEvent.getName() == "Min_ID_node_X" )
                        gui_blk_idNodo_minXY = int(theEvent.getStringValue());
                    if (theEvent.getName() == "Num_node_X" )
                        gui_blk_num_xmax = int(theEvent.getStringValue());
                     if (theEvent.getName() == "Num_node_Y" )
                        gui_blk_num_ymax = int(theEvent.getStringValue());
               }
      }
    else if (theEvent.getName() == "dt"){
      caseParams.dt = theEvent.getValue();
      ac.frame_time = theEvent.getValue();
    }

    if (caso == BLOQUE_VTK){
      // Frente
      if(theEvent.isFrom(r1)) {

        //for(int i=0;i<theEvent.getGroup().getArrayValue().length;i++) {
          //gui_activation_mode = int(theEvent.getGroup().getArrayValue()[i]);
          gui_activation_mode = int(r1.getValue());
        //}

       println("Activation mode: ", gui_activation_mode);

      }
      // Bloque central
      else
      if(theEvent.isFrom(chk_blk)) {

      for(int i=0;i<theEvent.getGroup().getArrayValue().length;i++) //commentato da giada il 02/12
          gui_blk_activated = int(theEvent.getGroup().getArrayValue()[i]);

        //  gui_blk_activated = int(chk_blk.getValue()); //giada 02/12
         //  gui_blk_activated = 2; //giada 02/12 parte b

         println("Activated Block: ", gui_blk_activated);
         println("ESTAMOS EN EL CASO DEL BLOQUE"); //when you select the activation for a block, this actually is read
      }

    }
    else

    if (caso == VENT){
      if(theEvent.isFrom(r1)) {
        caseParams.multi_view = !caseParams.multi_view;
        println("gui_multi_view mode: ", caseParams.multi_view);

       }
      // Bloque central
      else
      if(theEvent.isFrom(chk_blk)) {

      for(int i=0;i<theEvent.getGroup().getArrayValue().length;i++)
          gui_blk_activated = int(theEvent.getGroup().getArrayValue()[i]);

         println("Activated Block: ", gui_blk_activated);
         println("ESTAMOS EN EL CASO DEL BLOQUE"); //when you select the activation for a block, this actually is read
      }

    }

   if(theEvent.isFrom(chk_cmap)) {
         caseParams.show_cmap = !caseParams.show_cmap;
         println("Show Map: ", caseParams.show_cmap);
      }

   if(theEvent.isFrom(chk_hay_mesh)) {
         caseParams.hay_mesh = !caseParams.hay_mesh;
         println("Show Mesh: ", caseParams.hay_mesh);
      }

   if(theEvent.isFrom(use_restsurf)) {
         caseParams.restsurf = !caseParams.restsurf;
         println("Use Restitution Surface: ", caseParams.restsurf);
      }

   /*
   if(theEvent.isFrom(chk_show_path)) {
         caseParams.show_path = !caseParams.show_path;
         println("Show Path clicked: ", caseParams.show_path);
   }

   if(theEvent.isFrom(chk_show_grid) && (grid != null)) {
         caseParams.grid_enable = !caseParams.grid_enable;
         println("Show Grid clicked: ", caseParams.grid_enable);
    }
    */

  }

  public void play(int theValue) {
    gui_play = true;
    gui_pause = false;
  }

  public void pause(int theValue) {
    gui_pause = true;
    gui_play = false;
  }

  public void init(int theValue) {
    gui_init = true;
  }

  public void save(int theValue) {
    gui_save = true;
  }

  public void load(int theValue) {
    gui_load = true;
    InputTest it = new InputTest();
  }

}
