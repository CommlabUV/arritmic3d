import java.util.*;
import java.io.File;


class Bbox{
 float xmin,xmax;
 float ymin,ymax;
 float zmin,zmax;
 PVector center;
}

class AC {

  ArrayList <Node3> G;

  int                 nCeldas;
  IntList             Lab;
  IntList             Lbz;
  EventQueue Labp;

  float               max_path, max_st, beat_start_time;
  Node3               first;
  IntList             init_nodes;
  Boolean             full_activation;
  int                 num_beat;
  float               NextStimTime;
  float               LastStimTime;

  ArrayList<PVector>  dirs;    // Spatial directions
  IntList             active_dirs;

  Node3               map[][][];
  int                 bpl; // bloques por lado
  int                 bplX; // bloques por lado en X
  int                 bplY; // bloques por lado en Y

  boolean             detecta_reentradas;
  IntList             re_nodes;

  File               current_state_file, current_ev_file;
  String             path, case_path;
  Bbox               bbox;

  float             frame_time;
  float             t_next_frame;
  float             t_next_event;
  int               n_cells_updated;


  // Caso VENT y BLOQUE_VTK
  AC(CaseLoader c){
    if (caso == BLOQUE_VTK){
      bplX = c.bpl_CasoBloqueX;
      bplY = c.bpl_CasoBloqueY;
    }
    G = new ArrayList <Node3>();
    nCeldas = c.nCeldas;
    Lab = new IntList();
    Lbz = new IntList();
    re_nodes = new IntList();
    init_nodes = new IntList();
    int tamQueue = max(1,int(sqrt(nCeldas)));
    Labp = new EventQueue();
    println("Creada lista con prioridad. Capacidad para " + tamQueue + " celdas.");

    bbox = new Bbox();
    bbox.center = new PVector();

    // Files Path
    path = skecthPath;
    case_path = casePath;

    max_path = 0;
    max_st   = 0;
    first = null;
    full_activation = false;
    beat_start_time = 0;
    num_beat = 0;
    NextStimTime = stimFrecS1;
    LastStimTime = 0;
    detecta_reentradas = false;

    frame_time = caseParams.dt;
    t_next_frame = frame_time;
    n_cells_updated = 0;

    println("Construyendo Grafo .... ");

    // Se crea el grafo
    for (int id = 0 ; id < nCeldas; id++){
      PVector pos = new PVector(c.X.get(id),c.Y.get(id),c.Z.get(id));
      bbox.center.add(pos);
      PVector or = new PVector();
      int estado = -1;
      int tipo;
      int endo2Epi;
      if (caso == BLOQUE_VTK){
        endo2Epi = c.endo2Epi.get(id);
        //endo2Epi = 0; //// DESCOMENTAR Y DEFINIR SI EL ARCHIVO SCARTISSUE NO ES CORRECTO Y DEFINIMOS TODO EL BLOQUE IGUAL COMO: ENDO 0, MID 1 , EPI 2
        //tipo = 0; //////// DESCOMENTAR SI DEFINIMOS TODO EL BLOQUE IGUAL COMO: SANA 0, BZ 1 , CORE 2
        tipo = c.cellType.get(id);  //////// COMENTAR SI DEFINIMOS TODO EL BLOQUE IGUAL COMO: SANA 0, BZ 1 , CORE 2
      }
      // Si caso ventrículo leemos el valor etiquetado
      else{
        endo2Epi = c.endo2Epi.get(id);
        tipo = c.cellType.get(id);
      }

      // Guardamos lista de BZ para activar ectópicos como estímulos
      if (tipo == 1)
        Lbz.append(id);
      float P = 0.0;
      ///// SI BLOQUE_VTK DEFINIMOS ORIENTACIÓN DE FIBRAS
      if (caso == BLOQUE_VTK)
        //or = new PVector(0,1,0);
        or = caseParams.direccion_fibras_bloque;
      else if( c.oX.size() == 0 ||  c.oX.size() == 0 ||  c.oX.size() == 0 )
        or = new PVector(0,1,0);
      // Añadimos la Orient. de Fibras desde los datos leídos txt
      else
        or = new PVector(c.oX.get(id),c.oY.get(id),c.oZ.get(id));

      Node3 node = new Node3(id, pos, P, estado, or, or, tipo, endo2Epi);
      G.add(node);
    }

    println("Nodos del grafo creados: ",G.size() );
    bbox.center.div(G.size());
    bbox.xmin = c.X.min(); bbox.xmax = c.X.max();
    bbox.ymin = c.Y.min(); bbox.ymax = c.Y.max();
    bbox.zmin = c.Z.min(); bbox.zmax = c.Z.max();
    println("AC-bbox center: ", bbox.center);
    if (caseParams.grid_enable && grid != null)
      grid.setCenter(bbox.center);

    // Calculamos safety factor para todas las celdas excepto Core
    calcula_safetyFactor(c);

  }

  void conecta_vecinos(CaseLoader c){
    float nvec = 0;

    if (c != null) {  // Caso de fichero
        for (Node3 N: G){
         for (int id_vec: c.V.get(N.id)){
           Node3 vec = G.get(id_vec);
           // Incluímos vecinos Core también porque si no no se puede calcular el SafetyFactor pq solo tiene en cuenta vecinos Core respecto a Sanos o BZ
           //if (vec.tipo != 2) //Añadimos vecinos que no sean Core
             N.add_vecino(vec);
         }
         nvec+=N.Lv.size();
        }
        nvec=nvec/G.size();
    }
    println("Numero medio de vecinos .... ",nvec);

  }

  void calcula_safetyFactor(CaseLoader c){
    if (c != null) {  // Caso de fichero
      int cont = 0;
      for (Node3 N: G){
        // Solo lo calculamos si el nodo no es Core
        if (N.tipo != 2){
          int num_vec = c.V.get(N.id).size();
          // Nos aseguramos de hacerlo solo si tiene vecinos
          if (num_vec > 0){
            float sum_vec = 0.0;
            float sanos_vec = 0.0;
            // Opción A: implementación Full neighbourhood
            if (safety_factor_full){
              float peso_vecino = 1;
              sum_vec = peso_vecino; // Se cuenta él mismo inicialmente para suma total vecinos
              sanos_vec = peso_vecino;  // Se cuenta él mismo inicialmente para suma células sanas que conducen
              for (int id_vec: c.V.get(N.id)){
                Node3 vec = G.get(id_vec);
                sum_vec+=peso_vecino; // Sumamos vecino al total
                if (vec.tipo != 2) //Sumamos al cálculo vecinos sanos y BZ que no sean Core
                  sanos_vec+= peso_vecino;
              }
            }

            // Opción B: implementación “Skinned” elements (Número de vértices compartidos, él mismo 8 vert, vecino de 4 vert,
            // vecino de 2 vert y vecino de 1 vert)
            else {
              float peso_vecino_4vert = 4;
              float peso_vecino_2vert = 2;
              float peso_vecino_1vert = 1;
              sum_vec = peso_vecino_4vert*2; // Se cuenta él mismo inicialmente en valor doble  para suma total
              sanos_vec = peso_vecino_4vert*2;  // Se cuenta él mismo inicialmente en valor doble para suma células sanas que conducen
              PVector dir_normZ = new PVector(0,0,1); // Vector normal eje Z
              PVector dir_normY = new PVector(0,1,0); // Vector normal eje Y
              PVector dir_normX = new PVector(1,0,0); // Vector normal eje X

              /* Calculamos 3 productos escalares entre vector de celda a vecino y el vector normal a cada eje X, Y, Z,
                 para distinguir vecino que comparten 4 vértices, 2 o 1.
                 Si en los resultados de los 3 productos escalares obtenemos:
                        -> 2 ceros(comparten 4 vértices)
                        -> 1 cero(comparten 2 vértices)
                        -> 0 ceros(comparten 1 vértice)
              */
              for (int id_vec: c.V.get(N.id)){
                Node3 vec = G.get(id_vec);
                // Calculamos el producto escalar entre vector de celda a vecino y respecto a los tres ejes, X, Y, Z
                // y acumulamos el número de ceros
                PVector vec_vecino = PVector.sub(vec.pos, N.pos).normalize();
                int num_zeros = 0;
                float pos_vecZ = abs(dir_normZ.dot(vec_vecino));
                if (pos_vecZ == 0)
                  num_zeros++;
                float pos_vecY = abs(dir_normY.dot(vec_vecino));
                if (pos_vecY == 0)
                  num_zeros++;
                float pos_vecX = abs(dir_normX.dot(vec_vecino));
                if (pos_vecX == 0)
                  num_zeros++;

                // Le damos el peso respecto a la posición de cada vecino
                // Si 0 ceros(comparten 1 vértice)
                if (num_zeros == 0){
                  sum_vec += peso_vecino_1vert; // Sumamos vecino al total
                  if (vec.tipo != 2) //Sumamos al cálculo vecinos sanos y BZ que no sean Core
                    sanos_vec += peso_vecino_1vert;
                }
                // Si 1 cero(comparten 2 vértices)
                else if (num_zeros == 1){
                  sum_vec += peso_vecino_2vert; // Sumamos vecino al total
                  if (vec.tipo != 2) //Sumamos al cálculo vecinos sanos y BZ que no sean Core
                    sanos_vec += peso_vecino_2vert;
                }
                // Si 2 ceros(comparten 4 vértices)
                else{
                  sum_vec += peso_vecino_4vert; // Sumamos vecino al total
                  if (vec.tipo != 2) //Sumamos al cálculo vecinos sanos y BZ que no sean Core
                    sanos_vec += peso_vecino_4vert;
                }
              }
            }
            N.safety_factor = sanos_vec/sum_vec;
            //N.safety_factor = 1;


            if (N.safety_factor != 1.0){
              cont++;
              //println("SAFETY", cont,N.id , N.safety_factor);
            }


          }
        }
      }
      println("SAFETY Num Nodes", cont);
    }
  }


  void init_beat(float t)
  {
     first = null;
     max_path = 0;
     max_st   = 0;
     beat_start_time = t;
  }


  void reset(){

    Lab.clear();
    Labp.clear();
    init_nodes.clear();

    max_path = 0;
    max_st   = 0;
    first = null;
    full_activation = false;
    beat_start_time = 0;
    num_beat = 0;
    NextStimTime = stimFrecS1;
    LastStimTime = 0;
    detecta_reentradas = false;

    t_next_frame = frame_time;

    for (Node3 n: G)
       n.reset();
  }

  void draw_path(Node3 Root)
  {
      IntList Lpath = Root.getPath();

      for (int n:Lpath){
        stroke(random(0,100));
        Node3 N = G.get(n);
        point(N.pos.x,N.pos.y,N.pos.z);
      }

  }

  // Muestra el camino mas largo
  void show_lpath()
  {
    strokeWeight(0.35);

    if (first != null)
      draw_path(ac.first);

    strokeWeight(caseParams.voxel_size);
  }

  // Activamos parche de bloque por NODOS solo una vez cuando se llama desde main
  float activa_parcheBloqueNodos(boolean event_mode){
    float t = INFINITO;
    println("ACTIVAMOS BLK EN t: ", tiempo_transcurrido);
    // Limpiamos la lista de nodos de activación ya que solo falta activar el parche
    init_nodes.clear();
    // Los nodos tienen una fila más que los elementos de profundidad
    int capasZNodo = cas.capasZCasoBloque+1;
    // Los nodos por lado son uno más que los elementos
    int npl = bplY + 1;
    int gui_blk_num_ymax_Nodos = gui_blk_num_ymax +1;
    int gui_blk_num_xmax_Nodos = gui_blk_num_xmax +1;
    int idY = gui_blk_idNodo_minXY;
    for(int i = 0; i < gui_blk_num_xmax_Nodos;i++){
      for(int j = 0; j < gui_blk_num_ymax_Nodos*capasZNodo; j++){
        int id_blk = idY-j;
        init_nodes.append(id_blk);
      }
      idY+=(npl*capasZNodo);
    }

    println("PREPARADO PARCHE BLOQUE - Num lista INIT",init_nodes.size());
    int hay_nodos_refractarios = 0;
    int minNodo = init_nodes.min();
    int maxNodo = init_nodes.max();
    println("MIN MAX NODO: ", minNodo, maxNodo);
    for(int i : init_nodes) {
      Node3 init_node = G.get(i);
      if ( i == minNodo || i == maxNodo )
        println("ID - COORD NODO: ",i,init_node.pos );

      /*
      // OPT 2: Las activamos si están desactivadas o apd 70
      float tvida_70 = 0;
      float apd70 = init_node.apd * 0.7;
      // Si activada comprobamos su tvida_70
      if (init_node.estado == 2)
        tvida_70 = tiempo_transcurrido - init_node.start_time;
      if((init_node.estado != 2 || tvida_70 >= apd70)){
      */
      // OPT 1: Las activamos tanto si están desactivadas o no
        init_node.activar_BLK(Labp);  // NEW_PQ: check
        t = tiempo_transcurrido;
      //}else
        //hay_nodos_refractarios++;
    }
    if (hay_nodos_refractarios > 0)
      println("La activación no se producirá en "+hay_nodos_refractarios+" de "+init_nodes.size() + " nodos porque su estado es 2.");

    return t;
  }

  float activacion(boolean event_mode){
    float t = INFINITO;
    if( num_beat < nStimsS1 + nStimsS2)
      if (tiempo_transcurrido >= NextStimTime ){
          num_beat+=1;
          LastStimTime = tiempo_transcurrido;
          println("** Estímulo en T ", tiempo_transcurrido, " ms");
          int hay_nodos_refractarios = 0;
          for(int i : init_nodes) {
            Node3 init_node = G.get(i);
            if( init_node.estado != 2 )
            {
              Evento new_ev = init_node.en_espera(0.0,0,null,num_beat,true);
              if(new_ev != null) {
                Labp.add(new_ev);  // NEW_PQ: check
                t = tiempo_transcurrido;
              }
            }else
              hay_nodos_refractarios++;
          }
          if (hay_nodos_refractarios > 0) {
            println("La activación no se producirá en "+hay_nodos_refractarios+" de "+init_nodes.size() + " nodos porque su estado es 2.");
            // Si estamos en multiSim y S2 es demasiado temprano y no va a activar nodos, activamos variable de aviso para resetear y pasar a la siguiente simulación con nuevo S2
            if (multiSim && hay_nodos_refractarios == init_nodes.size()) {
              failed_S2 = true;
            }
          }
          if(num_beat < nStimsS1) // Los dos primeros estímulos a BCL largo
            NextStimTime += stimFrecS1;
          else
            NextStimTime += stimFrecS2;

          init_beat(tiempo_transcurrido);
      }

    return t;
  }

  void update(){

    t_next_event = INFINITO;

      if(!Labp.isEmpty()){
        Evento ev = Labp.poll();  // NEW_PQ: check
        Node3 n = G.get(ev.id);
        float new_t = ev.t;
        if(new_t < tiempo_transcurrido){
          println("\n\n\n\n\n          CUIDADO t="+tiempo_transcurrido+" mayor que   ev.t="+new_t);
          println("\n\n\n\n\n");
        }
        tiempo_transcurrido = new_t;
        n.dispara_evento(ev,Labp);  // NEW_PQ: check
        n_cells_updated++;
      }

      if(!Labp.isEmpty()) {
        Evento ev = Labp.peek(); // NEW_PQ: check
        t_next_event = ev.t;
        if(t_next_event < tiempo_transcurrido)
        {
          println("\n\n\n\n\n");
          println("\n          ERROR: nos hemos saltado un evento!");
          println("\n\n\n\n\n");
        }
      }

    if(Float.isNaN(t_next_event))
      println("  "+t_next_event);

 }



 void draw_insert(int mode, Boolean show_graph) {

   strokeWeight(caseParams.voxel_size);
   // Primero se pintan las activas, porque cuesta mucho menos.
   // Las inactivas se pintan en un bucle independiente al final de la función,
   // sólo si está activo su pintado.
   if ( Lab.size() !=0 ){
     for (int id_N: Lab) {
       if (random(0.0,1.0) < (1.0/caseParams.pinta_cada_n)) // En promedio, pinta uno de cada n
       {
         Node3 N = G.get(id_N);
          if (N.esperando == false)
           drawNode(N,mode,caseParams.tmode,show_graph);
       }
     }
   } else {

     int n = 0;
     for (Node3 N: G) {
      if ( N.tipo < 2 ) { // Si tipo no es core
        if (n == caseParams.pinta_cada_n){
           if( (caseParams.tmode == 2 ) || ( caseParams.tmode == N.tipo) ){
             N.tvida = N.t_proxima_desactivacion - tiempo_transcurrido;
             drawNode(N,mode,caseParams.tmode,show_graph);
           }
           n = 0;
        }
        n++;
      }
     }
   }

   // n_cells_updated nos dice cuántas celdas se han actualizado desde el último pintado.
   //n_cells_updated = 0;
 }


void drawNode(Node3 N,int mode, int visu, Boolean show_graph) {
    boolean frente_onda = false;
    boolean invisible = false;

    if (N.estado == -1){
      if(alphaInactive == true)
        stroke(0,0,0,36); // negro
      else
        return;
    } else {

      if (mode == 5) {
        if (N.estado == 2) {
          if (tiempo_transcurrido - N.start_time < caseParams.tam_frente)
            frente_onda = true;
          if (tiempo_transcurrido - N.start_time > N.apd - caseParams.tam_frente )
            frente_onda = true;
        }
      }


      if (mode == 0){ // Pintamos por estado
        switch(N.estado){
         case 2:     // Si activada y zona BZ --> verde
              if (N.tipo == 1)
                stroke(10,200,10, 80);
              else
                stroke(10,10, 200, 60);
              break;
         case 1:     // Si se acaba de activar --> rojo
              stroke(200,20,20,255);
              break;
         case 0:     // Si está esperando acaba de activar --> rojo oscuro
              stroke(180,5,5,255);
              break;

        }
      } else if(N.estado == 0) {
        return;
      } else {
        float r, g, b;
        float valor;

        if (mode == 1 || mode == 5){  // visualizacion del tiempo de vida
          if (visu == 2){
            if (N.apd == 0.0)
              valor = 0;
            else
              valor = 350*N.tvida/N.apd;
            caseParams.rango_visual = 350;
            caseParams.rango_min = 0;
            invisible = false;
          }
          else
          {
            if (N.tipo == visu)
            {
              valor = N.tvida;
              caseParams.rango_visual = 350;
              caseParams.rango_min = 0;
              invisible = false;
            }
            else
            {
              valor = 0.0;
              invisible = true;
            }

          }

        } else if (mode == 2 ) { // visualizacion del APD
          caseParams.rango_visual = 350 ; //290  //310  //315 APD90
          caseParams.rango_min = 290;      //190  //220
          valor = map(N.apd,caseParams.rango_min,caseParams.rango_visual,0,caseParams.rango_visual);

        } else if (mode == 3 ) { // visualización del DI
          caseParams.rango_visual = 300;
          caseParams.rango_min = 0;
          valor = map(N.di,caseParams.rango_min,caseParams.rango_visual,0,caseParams.rango_visual);

        } else if (mode == 6 ) { // visualización del periodo de activacion
          caseParams.rango_visual = 600;
          caseParams.rango_min = 290;
          valor = map(N.periodo_activacion, caseParams.rango_min,caseParams.rango_visual,0,caseParams.rango_visual);

        } else  { // mode == 4 -> visualización CV
          caseParams.rango_visual = 100;
          caseParams.rango_min = 0;
          if(N.start_time - LastStimTime > 0)
            valor = (N.lpath/(N.start_time - LastStimTime))*100; //porque el rango es entero
          else
            valor = 0.0;

        }

        if(valor > caseParams.rango_visual/2) {
          r = map(valor, caseParams.rango_visual, caseParams.rango_visual/2, 255, 0);
          b = 0;
          g = map(valor, caseParams.rango_visual, caseParams.rango_visual/2, 0, 255);
        }else{
          r =  0;
          b = map(valor, caseParams.rango_visual/2, 0, 0,255);
          g = map(valor, caseParams.rango_visual/2, 0, 255, 0);
        }

        stroke(r, g, b);
      }
    }

    if(mode != 5 || frente_onda) {
      if (invisible == false){
        // Separamos un poco en el eje Z el dibujado para que se visualice
        if (caso == BLOQUE_VTK)
          point(N.pos.x,N.pos.y,N.pos.z+0.09);
        else
          point(N.pos.x,N.pos.y,N.pos.z);
      }

      if (show_graph)
        for (int i=0;i<N.Lv.size();i++){
          Node3 vec = N.Lv.get(i);
          line(N.pos.x,N.pos.y,N.pos.z, vec.pos.x,vec.pos.y,vec.pos.z);
        }
    }


   // Marcamos las re-entradas con flashes a todo color
   for (int i: re_nodes){
     Node3 Nr = G.get(i);
     strokeWeight(0.8+random(0.5));
     stroke(random(255),random(255),random(255));
     point(Nr.pos.x,Nr.pos.y,Nr.pos.z);

   }
   strokeWeight(caseParams.voxel_size);

  }



    PVector get_errors()
    {
      float ed = 0;
      float et = 0;
      for (Node3 n: G){
         ed+= n.derror;
         et+= n.terror;
      }
      return new PVector(ed,et);
    }

    void draw_derror(int emode) {

      strokeWeight(caseParams.voxel_size);

      for (int id_N: Lab) {
          Node3 N = G.get(id_N);

          float val;
          if (emode == 0){
            val = map(N.derror, 0, 0.6, 0,255);
            stroke(val);
          }
          else{
            val = map(N.terror, 0, caseParams.dt, 0,255);
            stroke(val);
          }

          point(N.pos.x,N.pos.y,N.pos.z);


      }

  }

  void activa_frente(int mode){
    // Comprobamos si variable activa frente nodos definidos en txt está a true para ejecutar este tipo de frente
    if (caseParams.active_MultiIDs_extraI){
       // TODO: Cuando se integre la dirección de la propagación, los valores leídos en el txt están en un ArrayList<PVector> con nombre caseParams.dirProp_extraIMulti
       int id_i;
       for (int i = 0; i < caseParams.ids_extraIMulti.size(); i++){
         id_i = caseParams.ids_extraIMulti.get(i);
         // Comprobamos si id existe en el modelo
         if(id_i < ac.G.size()) {
           Node3 init_node = ac.G.get(id_i);
           //Solo los añadimos si no son Core para que no dé fallos de estado en los eventos
           if (init_node.tipo != 2){
             ac.init_nodes.append(id_i);
             init_node.en_espera(caseParams.delays_extraIMulti.get(i),0,null, 0,true);
             ac.Labp.add(init_node.siguienteEvento()); // NEW_PQ: check
           }
         }
         // Si no avisamos que id no existe en el modelo
         else
           println("ID "+ id_i + " no existe en el modelo");
       }
       println("Lista Init Nodes ID's: ", ac.init_nodes);
    }
    // Si no ejecutamos nodos de activación inicial dependiendo del mode (nodo único, frente vertical, frente horizontal)
    else {
      if (mode == 0){ // nodo foco
        int id_i;
        id_i = id_extraI;
        ac.init_nodes.append(id_i);
        Node3 init_node = ac.G.get(id_i);
        init_node.en_espera(gui_pdelay,0,null, 0,true);
        ac.Labp.add(init_node.siguienteEvento()); // NEW_PQ: check

      }
      // Frentes horizontales y verticales
      else if (mode == 1){
         // Los nodos tienen una fila más que los elementos
         int capasZNodo = cas.capasZCasoBloque +1;
         // Nodos por columna
         int nplY = bplY*capasZNodo + capasZNodo;

         // Frente Horizontal de izq -> der
         if (gui_stimFrec_hz > 0){
           int id_i;
             for(int i=0;i<nplY;i++){
               id_i = i;
               //Solo los añadimos si no son Core para que no dé fallos de estado en los eventos
               if(id_i < ac.G.size()) {
                 Node3 init_node = ac.G.get(id_i);
                 if (init_node.tipo != 2){
                   ac.init_nodes.append(id_i);
                   init_node.en_espera(gui_hdelay,0,null, 0,true);
                   ac.Labp.add(init_node.siguienteEvento()); // NEW_PQ: check
                 }
               }
             }
         }

         // Frente Vertical de abajo -> arriba
         if (gui_stimFrec_vt > 0) {
           int id_i;
           for (int k=0;k<capasZNodo;k++)
             for(int i=0;i<=bplX;i++){
               id_i = nplY*i+k;
               //Solo los añadimos si no son Core para que no dé fallos de estado en los eventos
               if(id_i < ac.G.size()) {
                 Node3 init_node = ac.G.get(id_i);
                 if (init_node.tipo != 2){
                   ac.init_nodes.append(id_i);
                   init_node.en_espera(gui_vdelay,0,null, 0,true);
                   ac.Labp.add(init_node.siguienteEvento()); // NEW_PQ: check
                 }
               }
             }
           }
        }
      }
    }


    void save_current_state()
    {
      PrintWriter output, output2;

      current_state_file = new File(ac.case_path+"estados/ac_state_"+str(tiempo_transcurrido)+".txt");

      output  = createWriter(current_state_file);

      // Var globales
      String header = str(tiempo_transcurrido)+'\t'+str(t_next_event)+'\t'+str(t_next_frame)+'\t'+str(n_cells_updated)+'\t'+str(t_fin_ciclo)+'\t';
      header+=str(stimFrecS1)+'\t'+str(nStimsS1)+'\t'+str(num_beat)+'\t'+str(LastStimTime)+'\t'+str(caseParams.min_pot_act);
      output.println(header);
      output.flush(); // Writes the remaining data to the file


      println(" Vamos a guardar "+G.size()+" nodos.");
      int saved_nodes = 0;

      // Var por nodo
      for (Node3 n: G){

        int idPadre = -1;
        if (n.padre != null)
          idPadre = n.padre.id;

        String int_line = str(n.id)+'\t'+str(n.estado)+'\t'+str(n.beat)+'\t'+str(idPadre)+'\t'+str(int(n.estimulo_externo));
        int_line += '\t' + str(n.n_activadores);

        output.println(int_line);
        output.flush(); // Writes the remaining data to the file

        String float_line = str(n.cv)+'\t'+str(n.apd)+'\t'+str(n.tvida)+'\t'+str(n.start_time)+'\t'+str(n.end_time)+'\t';
        float_line += str(n.di)+'\t'+str(n.lpath)+'\t'+str(n.t_proxima_activacion)+'\t'+str(n.t_proxima_desactivacion)+'\t'+str(n.t_proximo_evento);
        float_line += '\t' + str(n.suma_tiempos_activadores);
        float_line += '\t' + str(n.tiempo_activador);
        if(n.suma_activadores != null)
        {
          float_line += '\t' + str(n.suma_activadores.x) + '\t' + str(n.suma_activadores.y) + '\t' + str(n.suma_activadores.z);
          float_line += '\t' + str(n.foco_activador.x) + '\t' + str(n.foco_activador.y) + '\t' + str(n.foco_activador.z);
          float_line += '\t' + str(n.normal_frente.x) + '\t' + str(n.normal_frente.y) + '\t' + str(n.normal_frente.z);
        }else{
          float_line += "\t0.0\t0.0\t0.0";
          float_line += "\t0.0\t0.0\t0.0";
          float_line += "\t0.0\t0.0\t0.0";
        }

        output.println(float_line);
        output.flush(); // Writes the remaining data to the file
        if( saved_nodes%1000 == 0 )
          print(".");
        saved_nodes++;
      }

      output.close(); // Finishes the file
      println(" hecho.");

      ////////////////////////////////////////////7
      // Lista de eventos
      current_ev_file = new File(ac.case_path+"estados/ac_ev_"+str(tiempo_transcurrido)+".txt");
      output2 = createWriter(current_ev_file);

      println(" Vamos a guardar "+Labp.size()+" estados.");

      int saved = 0;
      for (Evento ev: Labp.tree){
         output2.println(str(ev.id)+'\t'+str(ev.t)+'\t'+str(ev.st));
         if( saved%1000 == 0)
           print(".");
         saved++;
      }
      println(" hecho.");

      output2.close(); // Finishes the file

      println("AC-STATE saved at T = ", str(tiempo_transcurrido));
      println("AC-STATE File: ", current_state_file );
      println("AC-EVENT File: ", current_ev_file );


    }


    void load_state(File fname)
    {

        // Cargamos de nuevo los parámetros para actuaizar cambios
        caseParams = new FileParams(casePath+"params.dat");

        if (fname.length() == 0)
          fname = current_state_file;


        BufferedReader reader = createReader(fname);
        String line = null;
        try {
          // Lee cabecera
          line = reader.readLine();
          if (line != null){
            String[] ac_vars = split(line, TAB);
            tiempo_transcurrido =  float(ac_vars[0]);
            t_next_event        =  float(ac_vars[1]);
            t_next_frame        =  float(ac_vars[2]);
            n_cells_updated     =  int(ac_vars[3]);
            t_fin_ciclo         =  float(ac_vars[4]);
            stimFrecS1          =  float(ac_vars[5]);
            nStimsS1            =  int(ac_vars[6]);
            num_beat            =  int(ac_vars[7]);
            LastStimTime        =  float(ac_vars[8]);
            caseParams.min_pot_act = float(ac_vars[9]);
            if(num_beat < nStimsS1) // Los dos primeros estímulos a BCL largo
              NextStimTime = LastStimTime+stimFrecS1;
            else
              NextStimTime = LastStimTime+stimFrecS2;
          }

          int id_nodo = 0;
          while ( id_nodo < G.size() && ((line = reader.readLine()) != null)) {
             String[] int_line = split(line, TAB);
             G.get(id_nodo).id                =  int(int_line[0]);
             G.get(id_nodo).estado            =  int(int_line[1]);
             G.get(id_nodo).beat              =  int(int_line[2]);
             int idPadre                      =  int(int_line[3]);
             if (idPadre >= 0)
               G.get(id_nodo).padre           =  G.get(idPadre);
             else
               G.get(id_nodo).padre           = null;
             G.get(id_nodo).estimulo_externo  =  boolean(int_line[4]);
             G.get(id_nodo).n_activadores     =  int(int_line[5]);


             line = reader.readLine();
             String[] float_line = split(line, TAB);

             G.get(id_nodo).cv                =  float(float_line[0]);
             G.get(id_nodo).apd               =  float(float_line[1]);
             G.get(id_nodo).tvida             =  float(float_line[2]);
             G.get(id_nodo).start_time        =  float(float_line[3]);
             G.get(id_nodo).end_time          =  float(float_line[4]);
             G.get(id_nodo).di                =  float(float_line[5]);
             G.get(id_nodo).lpath             =  float(float_line[6]);
             G.get(id_nodo).t_proxima_activacion      =  float(float_line[7]);
             G.get(id_nodo).t_proxima_desactivacion   =  float(float_line[8]);
             G.get(id_nodo).t_proximo_evento          =  float(float_line[9]);
             G.get(id_nodo).suma_tiempos_activadores  = float(float_line[10]);
             G.get(id_nodo).tiempo_activador          = float(float_line[11]);

             // Este bloque se leerá el último. Si hay que añadir algo, hay
             // que hacerlo antes y cambiar el 10.
             G.get(id_nodo).suma_activadores = new PVector();
             G.get(id_nodo).foco_activador = new PVector();
             G.get(id_nodo).normal_frente = new PVector();
             if(G.get(id_nodo).n_activadores != 0)
             {
                G.get(id_nodo).suma_activadores.x       = float(float_line[12]);
                G.get(id_nodo).suma_activadores.y       = float(float_line[13]);
                G.get(id_nodo).suma_activadores.z       = float(float_line[14]);
                G.get(id_nodo).foco_activador.x         = float(float_line[15]);
                G.get(id_nodo).foco_activador.y         = float(float_line[16]);
                G.get(id_nodo).foco_activador.z         = float(float_line[17]);
                G.get(id_nodo).normal_frente.x          = float(float_line[18]);
                G.get(id_nodo).normal_frente.y          = float(float_line[19]);
                G.get(id_nodo).normal_frente.z          = float(float_line[20]);
             }

             G.get(id_nodo).min_potencial = caseParams.min_pot_act;

             id_nodo+=1;
          }
          println("Restaurados "+str(id_nodo)+ " nodos :: estado actual T:"+str(tiempo_transcurrido));
          reader.close();
        }
        catch (IOException e) {
          e.printStackTrace();
        }


        // Fichero de eventos
        Labp.clear();
        if (fname.length() > 0){
          String name = fname.getName();
          String path = fname.getParent();
          current_ev_file = new File(path+"/ac_ev_"+name.substring(9));
        }
        BufferedReader reader2 = createReader(current_ev_file);

        try {
          while ((line = reader2.readLine()) != null) {
             String[] int_line = split(line, TAB);
             Evento ev = new Evento();
             ev.id    =  int(int_line[0]);
             ev.t     =  float(int_line[1]);
             ev.st    =  int(int_line[2]);
             // NEW_PQ: check. Assign ev to node[i]
             Labp.add(ev);
          }
          reader2.close();
        } catch (IOException e) {
          e.printStackTrace();
        }
        println("Restaurados "+str(Labp.size())+ " eventos :: estado actual T:"+str(tiempo_transcurrido));

    }


 }

