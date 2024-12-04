import peasy.PeasyCam;
import toxi.geom.*;
import toxi.geom.mesh.*;
import toxi.geom.mesh.subdiv.*;
import toxi.processing.*;
import processing.opengl.*;
import toxi.util.*;
import toxi.volume.*;
import java.util.Formatter;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.concurrent.TimeUnit;
import java.net.InetAddress;


final float INFINITO = 1e10;

/////////////////////////////////////////////

// Graphics: camera, mesh, etc.
Controles WinGui;
Params    WinParams;

PeasyCam cam;
ToxiclibsSupport gfx;
WETriangleMesh mesh1;

// Read the different case; 0 -> physiological case; 1 -> Simpler geometry (such as tissue)
final int VENT      = 0;
final int BLOQUE_VTK = 1;

// Case and Patient, are defined when loading the initial case parameter file in setup().
int caso;
String paciente;


// Variables options : implementation safety factor
boolean safety_factor_active = true;  // We activate the implementation of the safety factor blocking of (Bueno Orovio et al., 2008).
boolean safety_factor_full = false;   // Option A: Full neighbourhood implementation
boolean safety_factor_skinned = true; // Option B: implementation ‘Skinned’ elements (Number of shared vertices, itself 8 vert, vec 4 vert, vec 2 vert and vec 1 vert)

// Variables for recording the animation .case files lifetime
boolean rec_case = false;
int t_case;
int t_caseIni;
int count_steps;
boolean escara_case = false; // Variable to store in Ensight Lifetime file only BZ and Core nodes.

// Variables para grabar los archivos .case de mapas de activación en cada estímulo
boolean escara_caseA = false; // Variable para guardar en archivo Ensight de Mapas de activación solo los nodos de BZ y Core
boolean mid_A = false; // Variable para guardar en archivo Ensight de Mapas de activación solo los nodos tipo Mid
boolean rec_caseMAP = false;
int numBeat; // Número de estímulos independiente del num de estímulos real porque no siempre se empezará a grabar en t0
FloatList t_beats = new FloatList(); // Tiempos de activación de cada estímulo
FloatList a_times1 = new FloatList(); // Lista que guarda a lo largo del primer estímulo, los Tiempos de activación de cada nodo
FloatList a_times2 = new FloatList(); // Lista que guarda a lo largo del siguiente estímulo, los Tiempos de activación de cada nodo
FloatList a_times3 = new FloatList(); // Lista que guarda a lo largo del siguiente estímulo, los Tiempos de activación de cada nodo
FloatList a_times4 = new FloatList(); // Lista que guarda a lo largo del siguiente estímulo, los Tiempos de activación de cada nodo
FloatList a_times1reen = new FloatList(); // Lista que guarda tiempos y id's de posible reentrada
FloatList a_times2reen = new FloatList(); // Lista que guarda tiempos y id's de posible reentrada
FloatList a_times3reen = new FloatList(); // Lista que guarda tiempos y id's de posible reentrada
FloatList a_times4reen = new FloatList(); // Lista que guarda tiempos y id's de posible reentrada
FloatList a_vel1 = new FloatList(); // Lista que guarda a lo largo del primer estímulo, las velocidades de cada nodo
FloatList a_vel2 = new FloatList(); // Lista que guarda a lo largo del siguiente estímulo, las velocidades de cada nodo
FloatList a_vel3 = new FloatList(); // Lista que guarda a lo largo del siguiente estímulo, las velocidades de cada nodo
FloatList a_vel4 = new FloatList(); // Lista que guarda a lo largo del siguiente estímulo, las velocidades de cada nodo
int t_activacion = 800; // Tiempo de espera de guardar tiempos de activación para cada beat y registre todos los nodos aunque se estimule otro beat
int t_waitReen = 6000; // Tiempo de espera cuando ocurre una reentrada en MultiSim, para finalizar la simulación con reentrada sostenida y pasar a la siguiente simulación
float t_esperaActivacion = 400; // Tiempo de espera para activar variable de posible reentrada y avisar por terminal de tiempos muy altos desde el último beat si siguen activándose nodos después del t definido
float count_t_lista1 = 0; // Contador del tiempo de guardado de datos de activación de lalista 1
float count_t_lista2 = 0; // Contador del tiempo de guardado de datos de activación de lalista 2
float count_t_lista3 = 0; // Contador del tiempo de guardado de datos de activación de lalista 3
float count_t_lista4 = 0; // Contador del tiempo de guardado de datos de activación de lalista 3
float beat_lista1 = -1.0; //Guardamos el beat del estímulo de la lista 1
float beat_lista2 = -1.0; //Guardamos el beat del estímulo de la lista 2
float beat_lista3 = -1.0; //Guardamos el beat del estímulo de la lista 3
float beat_lista4 = -1.0; //Guardamos el beat del estímulo de la lista 3
Boolean lista1_open = false; // Variable que identifica si primera lista abierta guardando datos de activación
Boolean lista2_open = false; // Variable que identifica si segunda lista abierta guardando datos de activación
Boolean lista3_open = false; // Variable que identifica si tercera lista abierta guardando datos de activación
Boolean lista4_open = false; // Variable que identifica si tercera lista abierta guardando datos de activación
Boolean posible_reentrada1 = false; // Variable que advierte de comportamientos extraños con activaciones de tiempos muy altos que pueden generar reentrada
Boolean posible_reentrada2 = false; // Variable que advierte de comportamientos extraños con activaciones de tiempos muy altos que pueden generar reentrada
Boolean posible_reentrada3 = false; // Variable que advierte de comportamientos extraños con activaciones de tiempos muy altos que pueden generar reentrada
Boolean posible_reentrada4 = false; // Variable que advierte de comportamientos extraños con activaciones de tiempos muy altos que pueden generar reentrada
Boolean lista1_reen = false; // Variable que identifica si la posible reentrada está guardada en lista1, para antes de cerrarla obtener datos de mínimos t's de activación y demás
Boolean lista2_reen = false; // Variable que identifica si la posible reentrada está guardada en lista2, para antes de cerrarla obtener datos de mínimos t's de activación y demás
Boolean lista3_reen = false; // Variable que identifica si la posible reentrada está guardada en lista3, para antes de cerrarla obtener datos de mínimos t's de activación y demás
Boolean lista4_reen = false; // Variable que identifica si la posible reentrada está guardada en lista3, para antes de cerrarla obtener datos de mínimos t's de activación y demás
Boolean count1_reentrada = false; // Variable que identifica reentrada en cada lista por tiempos de activación mayores a 600. Imprimimos solo una vez el aviso
Boolean count2_reentrada = false; // Variable que identifica reentrada en cada lista por tiempos de activación mayores a 600. Imprimimos solo una vez el aviso
Boolean count3_reentrada = false; // Variable que identifica reentrada en cada lista por tiempos de activación mayores a 600. Imprimimos solo una vez el aviso
Boolean count4_reentrada = false; // Variable que identifica reentrada en cada lista por tiempos de activación mayores a 600. Imprimimos solo una vez el aviso

////////////////////////////////////////////
// Caso
CaseLoader cas;
FileParams caseParams;
String casePath, skecthPath;
boolean alphaInactive = false;
boolean multiSim; // Distintas simulaciones con todo el rango de parámetros detallados en params, dentro de una misma ejecución
//boolean casoCargado = false; // Variable que evita que si multiSim carguemos el caso en cada simulación y demás datos que no se van a modificar entre simulaciones
ArrayList<FloatList> parametersMulti; // Lista de listas de parámetros para multiSim
ArrayList<FloatList> parametersMultiCurrent; // Copia Lista de listas de parámetros para multiSim que se irán modificando en cada ejecución
// Parámetros para simulación
float   stimFrecS1;
float   stimFrecS2;
int     nStimsS1;
int     nStimsS2;
float   cv_memory;
float   apd_isot_ef;
int     id_extraI;
float   t_LastStim; // Guarda el tiempo en el que se activa el último estímulo para saber en el multiSim cuando reiniciar la simulación con el siguiente número de parámetros
boolean failed_S2 = false; // Variable que identifica si S2 fallido para reiniciar a siguiente simulación
int     numMultiSim; // Número de simulaciones que se ejecutarán en multiSim
int     countMultiSim = 1; // Contador de número de simulación actual en multiSim. Inicializamos a 1 pq 1ª simulación se lanza sin pasar directa
int     countMultiSimFail = 0; // Contador de simulaciones fallidas por no activación de S2
float   tCountNodesReen; // Variable que detecta posibles reentradas en nodos init al ser activados por celdas vecinas y no por estímulo inicial
int     countNodesReen; // Variable que cuenta los ciclos de reentrada
float   beatReen1; // Posible beat en el que se genera la reentrada
float   beatReen2; // Posible beat en el que se genera la reentrada
float   beatReen3; // Posible beat en el que se genera la reentrada
float   beatReen4; // Posible beat en el que se genera la reentrada
PrintWriter outputSims; // Writer donde se guaradn resultados si multiSim
int     countSimsReen; // Variable que cuenta nº de simulaciones con reentradas en multisim
// Guardamos tiempo inicial y final para calcular el tiempo de ejecución de todas las simulaciones
long    timeMillisIni;
long    timeMillisFin;
float   millisecondsSim = 0; // Contador de milisegundos simulados en total en multiSim para calcular tiempo real de simulación
StringList paramsReens; // Lista de parámetros que generan reentradas con número de ciclos de la misma
// Id's de nodos de inicio reentrada
int id1;
int id2;
int id3;
int id4;


/////////////////////////////////////////////
// Var Grafo y frente de onda (lista Lab)
AC     ac;
CGrid  grid;

////////////////////////////////////////////
// Var fisicas
float tiempo_transcurrido;
float t_fin_ciclo;
float ac_first_start_time, ac_LastStimTime, ac_first_lpath;

float apdMAX = 0;
int tipoMAX = 0;
int endo2EpiMAX = 0;
float tiempo_transcurridoMAX = 0;
int idMAX;

boolean  blk_NOactivado = true; // Variable que indica si ya se ha activado el parche del bloque para activarlo una sola vez
int      lado       = 100; // Distancia usada para el texto del cmap


///////////////////////////////////////////////////////////7

public void settings() {
    System.setProperty("jogl.disable.openglcore", "true");
    size(900,600, P3D);
  }

void initmesh(String mfile){
    if(!caseParams.hay_mesh)
      println("No mesh!");
    else{
      //Mesh1 Core
      mesh1 = new WETriangleMesh();
      TriangleMesh tmesh = (TriangleMesh) new STLReader().loadBinary(sketchPath(mfile), STLReader.TRIANGLEMESH);
      if(tmesh!=null){
        mesh1.addMesh(tmesh);
        gfx = new ToxiclibsSupport(this);
        println("Mesh loaded! ");
      } else {
        println("Failed to load mesh file "+mfile+". No mesh!");
        caseParams.hay_mesh=false;
      }
    }
}

void initAll() {
    // Antes de inicializar todo, si multiSim, comprobamos si hemos ejecutado la última simulación de parámetros para guardar txt de resultados y salir del programa
    if (multiSim && countMultiSim > numMultiSim){
      timeMillisFin = System.currentTimeMillis();
      long timeMillisTotal = (timeMillisFin - timeMillisIni);
      String hms = String.format("%02d:%02d:%02d",
            TimeUnit.MILLISECONDS.toHours(timeMillisTotal),
            TimeUnit.MILLISECONDS.toMinutes(timeMillisTotal) - TimeUnit.HOURS.toMinutes(TimeUnit.MILLISECONDS.toHours(timeMillisTotal)),
            TimeUnit.MILLISECONDS.toSeconds(timeMillisTotal) - TimeUnit.MINUTES.toSeconds(TimeUnit.MILLISECONDS.toMinutes(timeMillisTotal)));
      int simsOK = numMultiSim - countMultiSimFail;
      outputSims.println("\n\n\n\n**************************************************** ");
      outputSims.println("****************************************************\n  ");
      outputSims.println("RESULTADO FINAL DE "+ numMultiSim +" SIMULACIONES: DE "+ simsOK +" SIMULACIONES NO FALLIDAS "+ countSimsReen +" CON POSIBLE REENTRADA");
      float probabilidadVT = (float(countSimsReen)/float(simsOK))*100;
      println("PROBABILIDAD DE VT: "+ int(probabilidadVT)+"%");
      outputSims.println("\nPROBABILIDAD DE VT: "+ int(probabilidadVT) +"%\n");
      outputSims.println("\nTIEMPO DE EJECUCIÓN DE "+ numMultiSim +" SIMULACIONES: " + hms +"\n");
      outputSims.println(millisecondsSim/1000 + " segundos de simulación en "+ timeMillisTotal/1000 + " segundos\n");
      outputSims.println("Un segundo de simulación en " + (timeMillisTotal/1000)/(millisecondsSim/1000)+" segundos\n");
      outputSims.println("\nPARÁMETROS DE SIMULACIONES CON REENTRADA:\n\n ");
      for (String paramsRe:paramsReens)
        outputSims.println(paramsRe+"\n");

      outputSims.close(); // Finishes the file
      println("----> ARCHIVO de resultados simulaciones "+paciente+"_multiSimsRes.txt, creado en carpeta arritmic3D/multiSims_Res");
      println("----> MULTI SIMULACIÓN FINALIZADA !!!!! ");
      exit();
      return;
    }

    if (multiSim){
        outputSims.println("\n\n\n*********************************\n ");
        outputSims.println("SIMULACIÓN "+countMultiSim+" de "+numMultiSim);
        println("INICIAMOS SIMULACIÓN "+countMultiSim+" de "+numMultiSim+ " !!!!!");
        // Generamos los parámetros para la simulación
        paramSim();
    }
    // Inicializamos contadores
    tiempo_transcurrido = 0.0;
    // Variabls que detectan posibles reentradas en nodos init al ser activados por celdas vecinas y no por estímulo inicial
    tCountNodesReen = INFINITO;
    countNodesReen = 0;
    // Inicializamos Variables de listas de mapa de activación
    posible_reentrada1 = false;
    posible_reentrada2 = false;
    posible_reentrada3 = false;
    posible_reentrada4 = false;
    count1_reentrada = false;
    count2_reentrada = false;
    count3_reentrada = false;
    count4_reentrada = false;
    count_t_lista1 = 0;
    count_t_lista2 = 0;
    count_t_lista3 = 0;
    count_t_lista4 = 0;
    lista1_open = false;
    lista2_open = false;
    lista3_open = false;
    lista4_open = false;
    lista1_reen = false;
    lista2_reen = false;
    lista3_reen = false;
    lista4_reen = false;
    beat_lista1 = -1.0;
    beat_lista2 = -1.0;
    beat_lista3 = -1.0;
    beat_lista4 = -1.0;
    beatReen1    = 0; // Posible beat en el que se genera la reentrada
    beatReen2    = 0; // Posible beat en el que se genera la reentrada
    beatReen3    = 0; // Posible beat en el que se genera la reentrada
    beatReen4    = 0; // Posible beat en el que se genera la reentrada
    // Inicializamos los id's de nodos de reentrada a -1 para distinguir si id guardado por reentrada o no
    id1 = -1;
    id2 = -1;
    id3 = -1;
    id4 = -1;
    // Creamos el grafo para el caso y elegimos un nodo de estimulación inicial
    if(caso == BLOQUE_VTK){
      if (ac == null){
        ac = new AC(cas);
        ac.conecta_vecinos(cas);
        ac.calcula_safetyFactor(cas);
      }
      else
        ac.reset();

      // Carga del cjto inicial de nodos en funcion del gui
      ac.activa_frente(gui_activation_mode);
    }
    // Si VENT
    else{
      if (ac == null){
        ac = new AC(cas);
        ac.conecta_vecinos(cas);
        ac.calcula_safetyFactor(cas);
      }
      else
        ac.reset();

    //ADDED BY GIADA ON 18/03/2023
    //IN THIS WAY YOU CAN ACTIVATE S1 FROM A SET OF NODES IN THE ATRIUM TOO
    if (caseParams.active_MultiIDs_extraI){ //IF THE VARIABLE active_MultiIDs_extraI IN params.dat IS TRUE, THEN....
      println("siamo entrati nel caso dell'active_MultiIDs_extraI      ");
      ac.activa_frente(gui_activation_mode);
    }
    else{
      // Añadimos todos los nodos de estimulación inicial
      Node3 cat_ini = ac.G.get(id_extraI);
      ac.init_nodes.appendUnique(cat_ini.id);
      cat_ini.chusca_catheter(caseParams.radio_cateter,0); //
      println("Lista Init Nodes ID's: ", ac.init_nodes);
      if (caseParams.grid_enable)
        grid.addNode(ac.G.get(id_extraI));

      for(int id_i : ac.init_nodes) {
        Node3 init_node = ac.G.get(id_i);

        Evento ev = init_node.en_espera(0.0,0,null,0,true);
          if(ev != null){
            //println("init_node.en_espera non è null");
            ac.Labp.add(ev);
          }
      }
    }


    }
    // Si multiSim no visualizamos nada en ventana para que vaya más deprisa
    if (multiSim){
      caseParams.show_frente = true;
      caseParams.hay_mesh = true;
      caseParams.restsurf = true;
      caseParams.show_cmap = true;
      caseParams.show_gui = true;
    }

    // Una vez construído el AC, inicializamos las 3 listas de tiempos de activación a -1 (Listas usadas para generar el Ensight de Mapas de activación )
    // Y las 3 listas de valor velocidad las inicializamos a 0
    // Y las 3 listas con posibles mínimos tiempos de reentrada
    a_times1.clear();
    a_times2.clear();
    a_times3.clear();
    a_times4.clear();
    a_times1reen.clear();
    a_times2reen.clear();
    a_times3reen.clear();
    a_times4reen.clear();
    a_vel1.clear();
    a_vel2.clear();
    a_vel3.clear();
    a_vel4.clear();
    for (int i = 0; i < ac.G.size(); i++){
      a_times1.append(-1);
      a_times2.append(-1);
      a_times3.append(-1);
      a_times4.append(-1);
      a_times1reen.append(-1);
      a_times2reen.append(-1);
      a_times3reen.append(-1);
      a_times4reen.append(-1);
      a_vel1.append(0);
      a_vel2.append(0);
      a_vel3.append(0);
      a_vel4.append(0);
    }

    if (multiSim) {
      count_t_lista1 = int(tiempo_transcurrido - ac.LastStimTime); // Inicilizamos contador de tiempos de guardado de datos de activación actualizandolo dependiendo del tiempo transcurrido desde estímulo actual
      beat_lista1 = ac.LastStimTime; // Guardamos el beat al que corresponde el guardado de la lista 1
      lista1_open = true; // Ponemos a true la variable que identifica que la lista 1 està guardando datos
    }
    // Inicilizamos contadores
    tiempo_transcurrido = 0.0;
    ac.t_next_event = 0.0;

}


// Definimos los parámetros para cada simulación si Multisim
void paramSim() {
    boolean borrado = true;
    int count = 0;
    FloatList paramsCurrent = new FloatList();

    // Copimaos la lista de listas para que nos permita modificar parametersMulti durante la ejecución del siguiente for
    ArrayList<FloatList> parametersLists = new ArrayList<FloatList>();
    for (FloatList listParams : parametersMultiCurrent) {
        parametersLists.add(listParams.copy());
    }

    for (FloatList listParams : parametersLists){
        paramsCurrent.append(listParams.get(0));
        if (borrado){
            // ELiminamos de la lista el parámetro ya usado
            parametersMultiCurrent.get(count).remove(0);
        }
        // Si aún quedan parámetros de esta lista no damos orden a la siguiente lista a eliminar su primer valor
        if (parametersMultiCurrent.get(count).size() != 0)
            borrado = false;
        // Si ya hemos simulado con todos los parámetros de esta lista la volvemos a llenar con todos los valores iniciales y pasamos orden a la lista siguiente para que elimine su primer valor
        else {
            parametersMultiCurrent.get(count).append(parametersMulti.get(count).copy());
            borrado = true;
        }
        count++;
    }
    // Guardamos los parámetros para la actual simulación
    stimFrecS1 = paramsCurrent.get(0);
    stimFrecS2 = paramsCurrent.get(1);
    nStimsS1   = int(paramsCurrent.get(2));
    nStimsS2   = int(paramsCurrent.get(3));
    cv_memory  = paramsCurrent.get(4);
    apd_isot_ef= paramsCurrent.get(5);
    id_extraI  = int(paramsCurrent.get(6));
    outputSims.println("\n*********************************\n ");
    outputSims.println("PARÁMETROS SIMULACIÓN ACTUAL: \tstimFrecS1: "+stimFrecS1+", stimFrecS2: "+ stimFrecS2+", nStimsS1: "+ nStimsS1+", nStimsS2: "+ nStimsS2+", cv_memory: "+ cv_memory+", apd_isot_ef: "+ apd_isot_ef+", id_extraI: "+ id_extraI);
    outputSims.flush();
    println("PARÁMETROS SIMULACIÓN ACTUAL: stimFrecS1: ",stimFrecS1," stimFrecS2: ", stimFrecS2," nStimsS1: ", nStimsS1," nStimsS2: ", nStimsS2," cv_memory: ", cv_memory," apd_isot_ef: ", apd_isot_ef," id_extraI: ", id_extraI);

}


void setup() {
    println("Inicio setup() ...");

    // Creamos la camara
    if (cam == null)
      cam = new PeasyCam(this,500);

    skecthPath = sketchPath("");

    // Cargamos archivo que define el tipo de caso (VENT / BLOQUE_VTK) y el path al caso ubicado en la carpeta arritmic3D/casos
    BufferedReader readerInit = createReader(skecthPath+"paramsInit.dat");
    String line = null;
    try {
        // Saltamos las dos lineas de comentarios
        line = readerInit.readLine();
        line = readerInit.readLine();
        if ((line = readerInit.readLine()) != null){
          println("  |-> "+line);
          String[] params = split(line, ':');
          caso = int(params[1].trim());
        }
        if ((line = readerInit.readLine()) != null){
          println("  |-> "+line);
          String[] params = split(line, ':');
          String caseF = params[1].trim();
          casePath = skecthPath+"casos/"+caseF+"/";
          paciente = caseF;
        }
      readerInit.close();
    } catch (IOException e) {
      e.printStackTrace();
    }

    // Si bloque vtk ponemos la variable paciente como Bloque (esta variable solo se usa para generar los nombres de las carpetas y archivos de los Ensight)
    if (caso == BLOQUE_VTK)
      paciente = "Bloque";
    // Si le ponen a la carpeta caso un nombre distinto a Berruezo_p* ponemos un nombre genérico por si ponen espacios o caracteres extraños
    else if (!paciente.startsWith("Berruezo"))
      paciente = "Ventriculo";

    caseParams = new FileParams(casePath);

    if (cas == null)
      initmesh(casePath+caseParams.model_file);

    // Cargamos el caso
    println("Loading case ...");
    if (cas == null)
      cas = new CaseLoader(casePath, caseParams.model_file);


    // Si multiSim, definimos en listas todo el rango de parámetros definidos para cada simulación en el params
    multiSim = caseParams.multiSim;

    countSimsReen = 0;
    parametersMulti = new ArrayList<FloatList>();
    parametersMultiCurrent = new ArrayList<FloatList>();
    paramsReens = new StringList();

    if (multiSim){
      parametersMulti.add(caseParams.stimFrecS1Multi); // Frecuencia de estimulo S1
      parametersMulti.add(caseParams.stimFrecS2Multi); // Frecuencia de estimulo S2 S3
      parametersMulti.add(caseParams.nStimsS1Multi);   // Nª de estímulos de S1
      parametersMulti.add(caseParams.nStimsS2Multi);   // Nª de estímulos de S2
      parametersMulti.add(caseParams.cv_memoryMulti);  // % Memoria CV
      parametersMulti.add(caseParams.apd_isot_efMulti);//Efecto electrotónico. % que afecta
      parametersMulti.add(caseParams.id_extraIMulti);
      println("puntoooooooos   " + caseParams.id_extraIMulti);

      // Copiamos el array de listas inicial para poder modificarlo pero mantener las listas originales para poder recargar los datos
      // Y contamos el número de simulaciones que se generarán
      numMultiSim = 1;
      for (FloatList listParams : parametersMulti) {
        numMultiSim *= listParams.size();
        FloatList currentList = listParams.copy();
        parametersMultiCurrent.add(currentList);
      }
      timeMillisIni = System.currentTimeMillis();
      println("TIME MILLIS INI!!!!!!!!!!!!!!",timeMillisIni);
      println("MULTISim Nº de Simulaciones: ", numMultiSim);
      // Creamos y dejamos abierto archivo txt donde vamos guardando resultados a lo largo de las simulaciones
      // Si carpeta no existe se crea
      File simsres = new File(skecthPath+"multiSims_Res/"+paciente+"_multiSimsRes.txt");
      outputSims  = createWriter(simsres);
      outputSims.println("\n**************************************************** ");
      outputSims.println("****************************************************\n ");
      outputSims.println("\tRESULTADOS SIMULACIONES PACIENTE "+ paciente + "\n");
      outputSims.println("**************************************************** ");
      outputSims.println("****************************************************\n ");

      outputSims.println("PARÁMETROS DE SIMULACIONES: \n");
      for (int i=0; i < parametersMulti.size(); i++){
        FloatList data = parametersMulti.get(i);
        String params = " ";
        if (i == 4 || i == 5)
          for(float param: data){params+=(param+", ");}
        else
          for(float param: data){params+=(int(param)+", ");}
        params = params.substring(0, params.length()-2);
        params+= " ";
        if (i == 0)
          outputSims.println("\n\tFrecuencia Estímulo S1 -> " + params);
        if (i == 1)
          outputSims.println("\n\tFrecuencia Estímulo S2 -> " + params);
        if (i == 2)
          outputSims.println("\n\tNº de Estímulos S1 -> " + params);
        if (i == 3)
          outputSims.println("\n\tNº de Estímulos S2 -> " + params);
        if (i == 4)
          outputSims.println("\n\tMemoria CV % -> " + params);
        if (i == 5)
          outputSims.println("\n\tEfecto electrotónico % -> " + params);
        if (i == 6)
          outputSims.println("\n\tID's Nodos de estímulo inicial -> " + params);
      }
      outputSims.println("\n\nNº TOTAL DE SIMULACIONES: \t" + numMultiSim);
      outputSims.flush(); // Writes the remaining data to the file
    }
    // Si no estamos en multisim definimos los parámetros de simulación individuales de la simulación única
    else {
        stimFrecS1 = caseParams.stimFrecS1;
        stimFrecS2 = caseParams.stimFrecS2;
        nStimsS1   = caseParams.nStimsS1;
        nStimsS2   = caseParams.nStimsS2;
        cv_memory  = caseParams.cv_memory;
        apd_isot_ef= caseParams.apd_isot_ef;
        id_extraI  = caseParams.id_extraI;
        println("PARAMS SIMULACIÓN: stimFrecS1:",stimFrecS1," stimFrecS2: ", stimFrecS2," nStimsS1: ", nStimsS1," nStimsS2: ", nStimsS2," cv_memory: ", cv_memory," apd_isot_ef: ", apd_isot_ef," id_extraI: ", id_extraI);
    }
    initAll();

    // Grid:
    if (caseParams.grid_enable){
      float tam_grid = 100;
      float tam_celda = 5;
      grid = new CGrid(tam_grid, tam_celda);
    }

    println("End setup() ...");

    println("Keys: f(frente on/off), l(Num células activas) , g(grabar frames), m(visualiz modos), t(tipo tejido), <- ->(rotar vista),  c(grabar Ensight Life Time(case)), e(grabar solo Escara Ensight Life Time(case), x(grabar Ensight Activation Time and Velocity Map(case), y(grabar solo Nodos Mid Ensight Activation Time and Velocity Map(case), z(grabar solo Escara Ensight Activation Time and Velocity Map(case))");

    // Al final del Setup ... para que este todo cargado y pueda inicializar bien las variables del gui
    if (caseParams.show_gui && WinGui == null)
      WinGui = new Controles();

    // Si multiSim, no mostramos ventana de winparams para que el framerate no baje por el redibujado del tiempo
    if (caseParams.show_gui && WinParams == null && !multiSim)
      WinParams = new Params();

    println("Control-Window ... done!");

    // Esto debe hacerse lo último. Si tarda mucho en arrancar puede dar timeout
    // En algunos Mac el framerate debe ser una constante literal.
    frameRate(caseParams.fps);
}


void draw() {

  int t0 = millis();


   //////////////////////////////////////////////////

  // CTRL-PLAYER
   if (caseParams.show_gui){
      if (gui_init){
        gui_init = false;
        gui_play = true;
        // Actualizamos el valor del id nodo inicial por si se ha modificado en la GUI
        if (!multiSim){
          id_extraI = gui_id_start_cell;
          caseParams.id_extraI = gui_id_start_cell;
        }
        // Si Multisim evitamos llamar dos veces a initAll, desde el setup y desde aquí, ya que la GUI se oculta
        if (!multiSim)
          initAll();
        return;
      }

      if (gui_save && gui_pause){
        ac.save_current_state();
        gui_save = false;
      }

      if (!gui_play )
        return;

      // Var para WinParams
      if (ac.first != null){
        ac_first_start_time = ac.first.start_time;
        ac_LastStimTime     = ac.LastStimTime;
        ac_first_lpath      = ac.first.lpath;
      }
   }

  //////////////////////////////////////////////////

  colorMode(RGB, 100);
  lights();
  background(100);

  /////////////////////////////////////////////////

  // DIFUSION

  // SI caso Bloque_vtk. Si activacion parche a 1 Activamos parche de bloque en un tiempo en ms exacto definido y como últimos nodos
  // de activación ya que limpia init_nodes antes de añadir parche
  if (caso == BLOQUE_VTK){
    if (gui_blk_activated == 1){
      float t_active_blk = 318; /// tiempo en milisegundos de activación parche bloque   Elvira -> 1320
      //per S1 = 150 -- t actt= 435 fa diffusione normale
      // per S1 = 150 -- t act = 430 fa un inizio di rotore
      //ultimo 10/01:  per S1 = 150 da rotore
      if (tiempo_transcurrido >= t_active_blk && blk_NOactivado){
        //float t = ac.activa_parcheBloque(true);
        float t = ac.activa_parcheBloqueNodos(true);
        // Ponemos a false para que el parche solo se active una vez
        blk_NOactivado = false;
        ac.init_nodes.clear();
        if(t < ac.t_next_event)
          ac.t_next_event = t;
      }
    }
  }
  //GIADA
    // SI caso CENT. Si activacion parche a 1 Activamos parche de bloque en un tiempo en ms exacto definido y como últimos nodos
  // de activación ya que limpia init_nodes antes de añadir parche
  if (caso == VENT){
    if (gui_blk_activated == 1){
      float t_active_blk = 320; /// tiempo en milisegundos de activación parche bloque   Elvira -> 1320
      if (tiempo_transcurrido >= t_active_blk && blk_NOactivado){
        //float t = ac.activa_parcheBloque(true);
        float t = ac.activa_parcheBloqueNodosVent(true);
        // Ponemos a false para que el parche solo se active una vez
        blk_NOactivado = false;
        ac.init_nodes.clear();
        if(t < ac.t_next_event)
          ac.t_next_event = t;
      }
    }
  }

  if( ac.num_beat < nStimsS1 + nStimsS2) {
      // Activamos nuevo estímulo
      if(ac.NextStimTime <= tiempo_transcurrido){
        float t = ac.activacion(true); //DOLORS
        // Inicializamos en cada estímulo variables de detección de nodos init activados por vecinos y el contador de ciclos de reentrada
        tCountNodesReen = INFINITO;
        countNodesReen = 0;
        // Si multiSim y S2 no activa vecinos reseteamos y pasamos a siguiente simulación
        if (multiSim && failed_S2){
          // Incrementamos contador de simulaciones
          countMultiSim++;
          // Incrementamos contador de simulaciones fallidas
          countMultiSimFail++;
          outputSims.println("\nABORTADA SIMULACIÓN POR S2 FALLIDO");
          println("ABORTAMOS SIMULACIÓN POR S2 FALLIDO!!!!!");
          // Acumulamos tiempo de simulación anterior para sumar el total antes de inicializar el tiempo_transcurrido a 0
          millisecondsSim += tiempo_transcurrido;
          initAll();
          failed_S2 = false;
          return;
        }
        t_LastStim = tiempo_transcurrido; // Guardamos tiempo de último estímulo para saber con multiSim cuándo reiniciar a nueva simulación
        // Si variable grabación mapas de acivación
        //if (rec_caseMAP || multiSim){
        if (rec_caseMAP){
          // Si primera lista no está abierta la abrimos y empezamos a guardar los datos de activación del actual beat en ella
          //if (!lista1_open && !lista2_open){
          if (!lista1_open){
            // Inicializamos la lista1 de tiempos de activación a -1 y velocidad a 0 para que los nodos que no se activen en el actual estímulo se queden con valores por defecto
            a_times1.clear();
            a_vel1.clear();
            for (int i = 0; i < ac.G.size(); i++){
              a_times1.append(-1);
              a_vel1.append(0);
            }
            count_t_lista1 = int(tiempo_transcurrido - ac.LastStimTime); // Inicilizamos contador de tiempos de guardado de datos de activación actualizandolo dependiendo del tiempo transcurrido desde estímulo actual
            beat_lista1 = ac.LastStimTime; // Guardamos el beat al que corresponde el guardado de la lista 1
            lista1_open = true; // Ponemos a true la variable que identifica que la lista 1 està guardando datos
            println("LISTA1 ABIERTA - BEAT-count", beat_lista1, count_t_lista1);
          }
          // Si primera lista abierta y sigue guardando datos, abrimos la lista2 y empezamos a guardar los datos de activación del actual beat en ella
          else if (!lista2_open){
            // Inicializamos la lista2 de tiempos de activación a -1 y velocidad a 0 para que los nodos que no se activen en el actual estímulo se queden con valores por defecto
            a_times2.clear();
            a_vel2.clear();
            for (int i = 0; i < ac.G.size(); i++){
              a_times2.append(-1);
              a_vel2.append(0);
            }
            count_t_lista2 = int(tiempo_transcurrido - ac.LastStimTime); // Inicilizamos contador de tiempos de guardado de datos de activación actualizandolo dependiendo del tiempo transcurrido desde estímulo actual
            beat_lista2 = ac.LastStimTime; // Guardamos el beat al que corresponde el guardado de la lista 1
            lista2_open = true; // Ponemos a true la variable que identifica que la lista 2 està guardando datos
            println("LISTA2 ABIERTA - BEAT - count", beat_lista2, count_t_lista2);
          }
          // Si segunda lista abierta y sigue guardando datos, abrimos la lista3 y empezamos a guardar los datos de activación del actual beat en ella
          else if (!lista3_open){
            // Inicializamos la lista3 de tiempos de activación a -1 y velocidad a 0 para que los nodos que no se activen en el actual estímulo se queden con valores por defecto
            a_times3.clear();
            a_vel3.clear();
            for (int i = 0; i < ac.G.size(); i++){
              a_times3.append(-1);
              a_vel3.append(0);
            }
            count_t_lista3 = int(tiempo_transcurrido - ac.LastStimTime); // Inicilizamos contador de tiempos de guardado de datos de activación actualizandolo dependiendo del tiempo transcurrido desde estímulo actual
            beat_lista3 = ac.LastStimTime; // Guardamos el beat al que corresponde el guardado de la lista 1
            lista3_open = true; // Ponemos a true la variable que identifica que la lista 3 està guardando datos
            println("LISTA3 ABIERTA - BEAT - count", beat_lista3, count_t_lista3);
          }
          // Si tercera lista abierta y sigue guardando datos, abrimos la lista4 y empezamos a guardar los datos de activación del actual beat en ella
          else if (!lista4_open){
            // Inicializamos la lista3 de tiempos de activación a -1 y velocidad a 0 para que los nodos que no se activen en el actual estímulo se queden con valores por defecto
            a_times4.clear();
            a_vel4.clear();
            for (int i = 0; i < ac.G.size(); i++){
              a_times4.append(-1);
              a_vel4.append(0);
            }
            count_t_lista4 = int(tiempo_transcurrido - ac.LastStimTime); // Inicilizamos contador de tiempos de guardado de datos de activación actualizandolo dependiendo del tiempo transcurrido desde estímulo actual
            beat_lista4 = ac.LastStimTime; // Guardamos el beat al que corresponde el guardado de la lista 1
            lista4_open = true; // Ponemos a true la variable que identifica que la lista 3 està guardando datos
            println("LISTA3 ABIERTA - BEAT - count", beat_lista4, count_t_lista4);
          }
          // Si no advertimos que nos hemos quedado sin listas para abrir
          else
            println("NO NOS QUEDAN LISTAS PARA ABRIR!!!!!!!!!!");

          // Guardamos t de estímulo en lista estímulos
          t_beats.append(ac.LastStimTime);
        }

        println("LLamo a activacion en t: ", tiempo_transcurrido, "NextStim", ac.NextStimTime);
        if(t < ac.t_next_event)
          ac.t_next_event = t;
        // Imprimimos APD Max en cada estímulo y lo inicializamos a 0 para volver a guardar datos en el siguiente estim
        println("APD MAX:", apdMAX,"TIPO ENDO(0)M(1)EPI(2):", endo2EpiMAX,"TIPO SANO(0)BZ(1)CORE(2):", tipoMAX, "t: ", tiempo_transcurridoMAX, "ID: ", idMAX);
        apdMAX = 0;
      }
  }
  //else if(ac.NextStimTime >= tiempo_transcurrido)
    //exit();
  // Si multiSim y t posterior o igual a último estímulo y ya hemos terminado S1-S2, incrementamos num_beat para que entre aquí en cada draw y esperamos 700ms
  // para revisar si quedan celdas activadas ante reentrada o no. Y reseteamos todo para lanzar la siguiente simulación con nuevos parámetros
  else if (multiSim && tiempo_transcurrido >= t_LastStim ){

      ac.num_beat+=1;
      if (tiempo_transcurrido > t_LastStim + 700){
          // Cerramos simulación si ya no hay eventos porque las celdas están todas desactivadas
          if (ac.Labp.isEmpty()){
            println("Cerramos porque no hay celdas activas en t: ", tiempo_transcurrido, countNodesReen);
            // Si no se han incrementado los ciclos de reentrada, no hay reentrada
            if(countNodesReen == 0)
              outputSims.println("\nFINALIZADA SIMULACIÓN SIN POSIBLE REENTRADA");
            else{
              outputSims.println("\nFINALIZADA SIMULACIÓN CON POSIBLE REENTRADA:\t "+countNodesReen+" Ciclos de reentrada");
              String idS = "";
              /*
              if (id1 != -1)
                idS += (" "+id1);
              if (id2 != -1)
                idS += (" "+id2);
              if (id3 != -1)
                idS += (" "+id3);
                */
              //paramsReens.append("SIMULACIÓN "+countMultiSim+" de "+numMultiSim+" \t-> "+countNodesReen +" ciclos reentrada \t-> Id's"+idS+"\t,\tstimFrecS1: "+stimFrecS1+", stimFrecS2: "+ stimFrecS2+", nStimsS1: "+ nStimsS1+", nStimsS2: "+ nStimsS2+", cv_memory: "+ cv_memory+", apd_isot_ef: "+ apd_isot_ef+", id_extraI: "+ id_extraI);
              paramsReens.append("SIMULACIÓN "+countMultiSim+" de "+numMultiSim+" \t-> "+countNodesReen +" ciclos reentrada \t-> \tstimFrecS1: "+stimFrecS1+", stimFrecS2: "+ stimFrecS2+", nStimsS1: "+ nStimsS1+", nStimsS2: "+ nStimsS2+", cv_memory: "+ cv_memory+", apd_isot_ef: "+ apd_isot_ef+", id_extraI: "+ id_extraI);
              countSimsReen++; // Incrementamos contador de simulaciónes con reentrada
            }
            // Incrementamos contadores de simulaciones para que en initAll finalice ejecución
            countMultiSim++;
            // Acumulamos tiempo de simulación anterior para sumar el total antes de inicializar el tiempo_transcurrido a 0
            millisecondsSim += tiempo_transcurrido;
            initAll();
            return;
          }
          // Si siguen habiendo celdas activándose por reentrada, esperamos el tiempo definido en variable t_waitReen, más después del último estímulo para obtener info de ciclos de reentrada
          // pero cerramos porque reentrada sostenida
          else if (tiempo_transcurrido > t_LastStim + t_waitReen){
            println("Cerramos aunque la reentrada sigue porque se ha sobrepasado el tiempo de "+t_waitReen+"ms en t: ", tiempo_transcurrido, countNodesReen);
            outputSims.println("\nFINALIZADA SIMULACIÓN CON REENTRADA SOSTENIDA:\t Finaliza simulación con reentrada sostenida tras "+countNodesReen+" ciclos reentrada");
            String idS = "";
            /*
            if (id1 != -1)
              idS += (" "+id1);
            if (id2 != -1)
              idS += (" "+id2);
            if (id3 != -1)
              idS += (" "+id3);
              */
            //paramsReens.append("SIMULACIÓN "+countMultiSim+" de "+numMultiSim+" \t-> Reentrada sostenida \t-> Id's"+idS+"\t,\tstimFrecS1: "+stimFrecS1+", stimFrecS2: "+ stimFrecS2+", nStimsS1: "+ nStimsS1+", nStimsS2: "+ nStimsS2+", cv_memory: "+ cv_memory+", apd_isot_ef: "+ apd_isot_ef+", id_extraI: "+ id_extraI);
            paramsReens.append("SIMULACIÓN "+countMultiSim+" de "+numMultiSim+" \t-> Reentrada sostenida \t-> \tstimFrecS1: "+stimFrecS1+", stimFrecS2: "+ stimFrecS2+", nStimsS1: "+ nStimsS1+", nStimsS2: "+ nStimsS2+", cv_memory: "+ cv_memory+", apd_isot_ef: "+ apd_isot_ef+", id_extraI: "+ id_extraI);
            countSimsReen++; // Incrementamos contador de simulaciónes con reentrada
            // Incrementamos contadores de simulaciones para que en initAll finalice ejecución
            countMultiSim++;
            // Acumulamos tiempo de simulación anterior para sumar el total antes de inicializar el tiempo_transcurrido a 0
            millisecondsSim += tiempo_transcurrido;
            initAll();
            return;
          }
      }
  }
  // tiempo_transcurrido se actualiza en el interior
  if(ac.t_next_event < ac.t_next_frame)
    do {
      ac.update();
    } while (tiempo_transcurrido <= ac.t_next_frame &&
                        ac.t_next_event <= ac.t_next_frame);
  else // Pero si no se actualiza AC, entonces lo incrementamos
    tiempo_transcurrido = ac.t_next_frame;

  ac.t_next_frame += ac.frame_time;

   // Si grabación de mapas de activación o multiSim vamos actualizando en cada update la lista de los tiempos de activación de los nuevos nodos que se hayan activado en lista1 o lista2 o listaReentrada
  //if (rec_caseMAP || multiSim){
  if (rec_caseMAP){
     // Comprobamos si las listas han superado su tiempo de guardado y las cerramos y generamos el .case, si no incrementamos el contador de tiempo
     if (lista1_open && count_t_lista1 > t_activacion){
       println("Cierro lista1 en t: ", tiempo_transcurrido);
       lista1_open = false;

       if (rec_caseMAP)
         generaEnsA_Map(a_times1, a_vel1);
     }
     else if (lista1_open)
       count_t_lista1 += caseParams.dt;
     if (lista2_open && count_t_lista2 > t_activacion){
       println("Cierro lista2 en t: ", tiempo_transcurrido);
       lista2_open = false;

       if (rec_caseMAP)
         generaEnsA_Map(a_times2, a_vel2);
     }
     else if (lista2_open)
       count_t_lista2 += caseParams.dt;

     if (lista3_open && count_t_lista3 > t_activacion){
       println("Cierro lista3 en t: ", tiempo_transcurrido);
       lista3_open = false;

       if (rec_caseMAP)
         generaEnsA_Map(a_times3, a_vel3);
     }
     else if (lista3_open)
       count_t_lista3 += caseParams.dt;

     if (lista4_open && count_t_lista4 > t_activacion){
       println("Cierro lista4 en t: ", tiempo_transcurrido);
       lista4_open = false;

       if (rec_caseMAP)
         generaEnsA_Map(a_times4, a_vel4);
     }
     else if (lista4_open)
       count_t_lista4 += caseParams.dt;


     // Recorremos todos los nodos para guardar tiempos de activación
     for (Node3 n: ac.G){
       // Si el nodo ya se ha activado
       if (n.start_time != INFINITO){
         // Si el beat que activó al nodo pertenece al tiempo de estímulo que guarda la lista. El valor guardado es el tiempo de activación
         // del correspondiente estímulo menos el tiempo del beat que guarda la lista, para que el rango de tiempo de todos los .ens sea el mismo
         // Si su beat de activación pertenece a la lista 1
         if (n.current_act_beat == beat_lista1 && lista1_open){
           // Comprobamos si no se ha guardado beat de reentrada para asegurar que guardamos primera 2ª activación y si ya existía dato de activación
           // anterior (valor distinto a inicialización -1) y el valor anterior es distinto al que vamos a guardar y por tanto es segunda activación en mismo beat para al final del bucle deducir nodo inicial reentrada
           /*
           if (a_times1.get(n.id) > -1 && a_times1.get(n.id) != (n.start_time - beat_lista1) && beatReen1 == 0 ){
             lista1_reen = true; // Ponemos a true variable que identifica que esta lista contiene datos de posibles reentrada
             a_times1reen.set(n.id, n.start_time - beat_lista1);
           }
           */
           //a_times1.set(n.id, n.start_time - beat_lista1);
           a_times1.set(n.id, n.start_time);
           //a_vel1.set(n.id, n.cv);
           //a_vel1.set(n.id, n.cv_real);
           a_vel1.set(n.id, n.apd);
           // Si el tiempo de activación mayor a 400 (variable definida al inicio) existe una activación fuera de lo normal y podría generar una reentrada. AVisamos solo una vez
           if (!posible_reentrada1 && n.start_time - beat_lista1 > t_esperaActivacion){
             posible_reentrada1 = true;
             println("TIEMPOS DE ACTIVACIÓN MUY ALTOS EN LISTA 1. ACTIVACIÓN FUERA DE LOS PARÁMETROS NORMALES !!! EN BEAT:  ", beat_lista1);
           }
         }
         // Si siguen habiendo nodos con el padre de activación del beat lista 1 pero hemos cerrado ya la lista y el dato que se está guardando no es el mismo que ya está guardado,
         // significa que posiblemente está ocuriendo una reentrada porque estamos dentro de la activación del beat en tiempos superiores a 600 ms. Advertimos de la situación
         else if (n.current_act_beat == beat_lista1 && !lista1_open && a_times1.get(n.id) != n.start_time - beat_lista1 && !count1_reentrada){
           println("POSIBLE REENTRADA!!! LISTA CERRADA DESPUÉS DE 600 ms PERO SIGUEN HABIENDO NODOS ACTIVÁNDOSE EN BEAT: ",beat_lista1);
           count1_reentrada = true;
         }

         // Si su beat de activación pertenece a la lista 2
         if (n.current_act_beat == beat_lista2 && lista2_open){
           // Comprobamos si no se ha guardado beat de reentrada para asegurar que guardamos primera 2ª activación y si ya existía dato de activación
           // anterior (valor distinto a inicialización -1) y el valor anterior es distinto al que vamos a guardar y por tanto es segunda activación en mismo beat para al final del bucle deducir nodo inicial reentrada
           /*
           if (a_times2.get(n.id) != -1 && a_times2.get(n.id) != (n.start_time - beat_lista2) && beatReen2 == 0){
             lista2_reen = true; // Ponemos a true variable que identifica que esta lista contiene datos de posibles reentrada
             a_times2reen.set(n.id, n.start_time - beat_lista2);
           }
           */
           //a_times2.set(n.id, n.start_time - beat_lista2);
           a_times2.set(n.id, n.start_time);
           //a_vel2.set(n.id, n.cv);
           //a_vel2.set(n.id, n.cv_real);
           a_vel2.set(n.id, n.apd);
           // Si el tiempo de activación mayor a 400 (variable definida al inicio) existe una activación fuera de lo normal y podría generar una reentrada. AVisamos solo una vez
           if (!posible_reentrada2 && n.start_time - beat_lista2 > t_esperaActivacion){
             posible_reentrada2 = true;
             println("TIEMPOS DE ACTIVACIÓN MUY ALTOS EN LISTA 2. ACTIVACIÓN FUERA DE LOS PARÁMETROS NORMALES !!! EN BEAT:  ", beat_lista2);
           }
         }
         // Si siguen habiendo nodos con el padre de activación del beat lista 2 pero hemos cerrado ya la lista, significa que posiblemente está ocuriendo una reentrada
         // porque estamos dentro de la activación del beat en tiempos superiores a 600 ms. Advertimos de la situación
         else if (n.current_act_beat == beat_lista2 && !lista2_open && a_times2.get(n.id) != n.start_time - beat_lista2 && !count2_reentrada){
           println("POSIBLE REENTRADA!!! LISTA CERRADA DESPUÉS DE 600 ms PERO SIGUEN HABIENDO NODOS ACTIVÁNDOSE EN BEAT: ",beat_lista2);
           count2_reentrada = true;
         }

         // Si su beat de activación pertenece a la lista 3
         if (n.current_act_beat == beat_lista3 && lista3_open){
           // Comprobamos si no se ha guardado beat de reentrada para asegurar que guardamos primera 2ª activación y si ya existía dato de activación
           // anterior (valor distinto a inicialización -1) y el valor anterior es distinto al que vamos a guardar y por tanto es segunda activación en mismo beat para al final del bucle deducir nodo inicial reentrada
           /*
           if (a_times3.get(n.id) != -1  && a_times3.get(n.id) != (n.start_time - beat_lista3) && beatReen3 == 0){
             lista3_reen = true; // Ponemos a true variable que identifica que esta lista contiene datos de posibles reentrada
             a_times3reen.set(n.id, n.start_time - beat_lista3);
           }
           */
           //a_times3.set(n.id, n.start_time - beat_lista3);
           a_times3.set(n.id, n.start_time);
           //a_vel3.set(n.id, n.cv);
           //a_vel3.set(n.id, n.cv_real);
           a_vel3.set(n.id, n.apd);
           // Si el tiempo de activación mayor a 400 (variable definida al inicio) existe una activación fuera de lo normal y podría generar una reentrada. AVisamos solo una vez
           if (!posible_reentrada3 && n.start_time - beat_lista3 > t_esperaActivacion){
             posible_reentrada3 = true;
             println("TIEMPOS DE ACTIVACIÓN MUY ALTOS  EN LISTA 3. ACTIVACIÓN FUERA DE LOS PARÁMETROS NORMALES !!! EN BEAT:  ", beat_lista3);
           }
         }
         // Si siguen habiendo nodos con el padre de activación del beat lista 3 pero hemos cerrado ya la lista, significa que posiblemente está ocuriendo una reentrada
         // porque estamos dentro de la activación del beat en tiempos superiores a 600 ms. Advertimos de la situación
         else if (n.current_act_beat == beat_lista3 && !lista3_open && a_times3.get(n.id) != n.start_time - beat_lista3 && !count3_reentrada){
           println("POSIBLE REENTRADA!!! LISTA CERRADA DESPUÉS DE 600 ms PERO SIGUEN HABIENDO NODOS ACTIVÁNDOSE EN BEAT: ",beat_lista3);
           count3_reentrada = true;
           }

                    // Si su beat de activación pertenece a la lista 3
         if (n.current_act_beat == beat_lista4 && lista4_open){
           // Comprobamos si no se ha guardado beat de reentrada para asegurar que guardamos primera 2ª activación y si ya existía dato de activación
           // anterior (valor distinto a inicialización -1) y el valor anterior es distinto al que vamos a guardar y por tanto es segunda activación en mismo beat para al final del bucle deducir nodo inicial reentrada
           /*
           if (a_times3.get(n.id) != -1  && a_times3.get(n.id) != (n.start_time - beat_lista3) && beatReen3 == 0){
             lista3_reen = true; // Ponemos a true variable que identifica que esta lista contiene datos de posibles reentrada
             a_times3reen.set(n.id, n.start_time - beat_lista3);
           }
           */
           //a_times3.set(n.id, n.start_time - beat_lista3);
           a_times4.set(n.id, n.start_time);
           //a_vel3.set(n.id, n.cv);
           //a_vel3.set(n.id, n.cv_real);
           a_vel4.set(n.id, n.apd);
           // Si el tiempo de activación mayor a 400 (variable definida al inicio) existe una activación fuera de lo normal y podría generar una reentrada. AVisamos solo una vez
           if (!posible_reentrada4 && n.start_time - beat_lista4 > t_esperaActivacion){
             posible_reentrada4 = true;
             println("TIEMPOS DE ACTIVACIÓN MUY ALTOS  EN LISTA 3. ACTIVACIÓN FUERA DE LOS PARÁMETROS NORMALES !!! EN BEAT:  ", beat_lista3);
           }
         }
         // Si siguen habiendo nodos con el padre de activación del beat lista 3 pero hemos cerrado ya la lista, significa que posiblemente está ocuriendo una reentrada
         // porque estamos dentro de la activación del beat en tiempos superiores a 600 ms. Advertimos de la situación
         else if (n.current_act_beat == beat_lista4 && !lista4_open && a_times4.get(n.id) != n.start_time - beat_lista4 && !count4_reentrada){
           println("POSIBLE REENTRADA!!! LISTA CERRADA DESPUÉS DE 600 ms PERO SIGUEN HABIENDO NODOS ACTIVÁNDOSE EN BEAT: ",beat_lista4);
           count4_reentrada = true;
           }
         }
       }
       // Lista reen 1 - Si hemos detectado posible reentrada y guardado tiempos de segundas activaciones buscamos origen reentrada con mínimo tiempo de activación
       // Guardamos mínimo tiempo y id del nodo
       if (lista1_reen){
         beatReen1 = beat_lista1; // Beat en el que se genera la reentrada y sirve para que no se vuelvan a guardar siguientes segundas activaciones
         float min_t_reentrada = INFINITO;
         int id_min = -1;
         for (int i= 0; i < a_times1reen.size(); i++){
           float a_time = a_times1reen.get(i);
           if (a_time != -1 )
             println("LISTA INI REEN 1 - Time - ID: ", a_time, i);
           // Descartamos los tiempos inicializados a -1 de nodos no activados
           if(a_time < min_t_reentrada && a_time > -1){
             min_t_reentrada = a_time;
             id_min = i;
           }
         }
         lista1_reen = false;
         id1 = id_min;
         println("EN MAPA LISTA 1 POSIBLE REENTRADA, MIN. TIEMPO ACTIVACION: ",min_t_reentrada,"EN ID: ", id_min," Y BEAT ", beat_lista1);
         if (multiSim)
           outputSims.println("\nNODO INICIO DE REENTRADA ( POR 2ª ACTIVACIÓN EN MISMO BEAT CON MÍNIMO t ): \tNodo ID "+ id_min+", 2º Tiempo de activación "+min_t_reentrada+", Beat "+ beat_lista1);
       }
       // Lista reen 2 - Si hemos detectado posible reentrada y guardado tiempos de segundas activaciones buscamos origen reentrada con mínimo tiempo de activación
       // Guardamos mínimo tiempo y id del nodo
       if (lista2_reen){
         float min_t_reentrada = INFINITO;
         int id_min = -1;
         for (int i= 0; i < a_times2reen.size(); i++){
           beatReen2 = beat_lista2; // Beat en el que se genera la reentrada y sirve para que no se vuelvan a guardar siguientes segundas activaciones
           // Descartamos los tiempos inicializados a -1 de nodos no activados
           float a_time = a_times2reen.get(i);
           if (a_time != -1 )
             println("LISTA INI REEN 2 - Time - ID: ", a_time, i);
           if(a_time < min_t_reentrada && a_time > -1){
             min_t_reentrada = a_time;
             id_min = i;
           }
         }
         lista2_reen = false;
         id2 = id_min;
         println("EN MAPA LISTA 2 POSIBLE REENTRADA, MIN. TIEMPO ACTIVACION: ",min_t_reentrada,"EN ID: ", id_min," Y BEAT ", beat_lista2);
         if (multiSim)
           outputSims.println("\nNODO INICIO DE REENTRADA ( POR 2ª ACTIVACIÓN EN MISMO BEAT CON MÍNIMO t ): \tNodo ID "+ id_min+", 2º Tiempo de activación "+min_t_reentrada+", Beat "+ beat_lista2);
       }

       // Lista reen 3 - Si hemos detectado posible reentrada y guardado tiempos de segundas activaciones buscamos origen reentrada con mínimo tiempo de activación
       // Guardamos mínimo tiempo y id del nodo
       if (lista3_reen){
         float min_t_reentrada = INFINITO;
         int id_min = -1;
         for (int i= 0; i < a_times3reen.size(); i++){
           beatReen3 = beat_lista3; // Beat en el que se genera la reentrada y sirve para que no se vuelvan a guardar siguientes segundas activaciones
           // Descartamos los tiempos inicializados a -1 de nodos no activados
           float a_time = a_times3reen.get(i);
           if (a_time != -1 )
             println("LISTA INI REEN 3 - Time - ID: ", a_time, i);
           if(a_time < min_t_reentrada && a_time > -1){
             min_t_reentrada = a_time;
             id_min = i;
           }
         }
         lista3_reen = false;
         id3 = id_min;
         println("EN MAPA LISTA 3 POSIBLE REENTRADA, MIN. TIEMPO ACTIVACION: ",min_t_reentrada,"EN ID: ", id_min," Y BEAT ", beat_lista3);
         if (multiSim)
           outputSims.println("\nNODO INICIO DE REENTRADA ( POR 2ª ACTIVACIÓN EN MISMO BEAT CON MÍNIMO t ): \tNodo ID "+ id_min+", 2º Tiempo de activación "+min_t_reentrada+", Beat "+ beat_lista3);
       }

              // Lista reen 3 - Si hemos detectado posible reentrada y guardado tiempos de segundas activaciones buscamos origen reentrada con mínimo tiempo de activación
       // Guardamos mínimo tiempo y id del nodo
       if (lista4_reen){
         float min_t_reentrada = INFINITO;
         int id_min = -1;
         for (int i= 0; i < a_times4reen.size(); i++){
           beatReen4 = beat_lista4; // Beat en el que se genera la reentrada y sirve para que no se vuelvan a guardar siguientes segundas activaciones
           // Descartamos los tiempos inicializados a -1 de nodos no activados
           float a_time = a_times4reen.get(i);
           if (a_time != -1 )
             println("LISTA INI REEN 4 - Time - ID: ", a_time, i);
           if(a_time < min_t_reentrada && a_time > -1){
             min_t_reentrada = a_time;
             id_min = i;
           }
         }
         lista4_reen = false;
         id4 = id_min;
         println("EN MAPA LISTA 4 POSIBLE REENTRADA, MIN. TIEMPO ACTIVACION: ",min_t_reentrada,"EN ID: ", id_min," Y BEAT ", beat_lista4);
         if (multiSim)
           outputSims.println("\nNODO INICIO DE REENTRADA ( POR 2ª ACTIVACIÓN EN MISMO BEAT CON MÍNIMO t ): \tNodo ID "+ id_min+", 2º Tiempo de activación "+min_t_reentrada+", Beat "+ beat_lista4);
       }
   }

  int t1 = millis() - t0;
  t_fin_ciclo = t1;

  ///////////////////////////////////////////////

  // DIBUJADO DE LA ESCENA
  if (!caseParams.multi_view){
    draw_scene(0);
  }
  else{
    draw_scene(1);
    cam.reset();
    draw_scene(2);
  }

 //////////////////////////////////////////////

}


void keyPressed()
{
  // Rotación de la vista del ventrículo
  if (key == CODED) {
    if (keyCode == LEFT) {
      cam.rotateY(0.26);
    } else if (keyCode == RIGHT) {
      cam.rotateY(-0.26);
    }
  }

  if (key == 'c'){ // Genera archivos .case para guardar animación de tiempo de vida
    rec_case = !rec_case;
    // Si primera vez que se pulsa c, generamos archivo .geo, inicializamos contador de steps (t_case) con tiempo real
    // y guardamos t inicial para calcular al final num de steps
    if (rec_case){
      println(" Iniciada grabación Ensight");
      generaGeo("Case");
      t_case = int(tiempo_transcurrido)-1;// Le restamos 1 porque la variable se incrementa en cada dibujado
      t_caseIni = t_case+1; // Añadimos 1 que será el t de inicio real
      count_steps = 0; // Inicializamos contador de steps para definir el nombre de cada archivo .ens
      println(" Iniciada grabación archivos .ens");
    }
    // Si pulsada tecla 2ª vez para final de grabación, calculamos steps guardados y generamos archivo .case
    else{
      int steps = t_case - t_caseIni;
      generaCase(steps);
      println(" Finalizada grabación Ensight");
    }
  }

  if (key == 'x'){ // Genera archivos .case para guardar mapas de activación en cada estímulo nuevo
    rec_caseMAP = !rec_caseMAP;
    // Si primera vez que se pulsa x, generamos archivo .geo y inicializamos contador de estímulos y abrimos lista 1
    if (rec_caseMAP){
      println(" Iniciada grabación Ensight Mapa de activación");
      generaGeo("A_Map");
      numBeat = 0; // Inicializamos contador de estímulos para nombre de archivos ens desde 0
      count_t_lista1 = int(tiempo_transcurrido - ac.LastStimTime); // Inicilizamos contador de tiempos de guardado de datos de activación actualizandolo dependiendo del tiempo transcurrido desde estímulo actual
      beat_lista1 = ac.LastStimTime; // Guardamos el beat al que corresponde el guardado de la lista 1
      lista1_open = true; // Ponemos a true la variable que identifica que la lista 1 està guardando datos
      println("LISTA1 ABIERTA - BEAT", beat_lista1);
      // Añadimos el estímulo actual en lista estímulos para rellenar .case
      t_beats.append(ac.LastStimTime);
    }
    // Si pulsada tecla 2ª vez para final de grabación generamos archivo .case
    else{
      // Generamos el último archivo .ens si hay listas aun abiertas
      if (lista1_open){
        lista1_open = false;
        generaEnsA_Map(a_times1, a_vel1);
      }
      if (lista2_open){
        lista2_open = false;
        generaEnsA_Map(a_times2, a_vel2);
      }
      if (lista3_open){
        lista3_open = false;
        generaEnsA_Map(a_times3, a_vel3);
      }
     if (lista4_open){
        lista4_open = false;
        generaEnsA_Map(a_times4, a_vel4);
      }
      // Generamos .case con los datos de los .ens generados
      generaCaseA_Map();
      println(" Finalizada grabación Ensight Mapa de activación");
    }
  }

  //  Activamos la variable para que el grabado de .ens de tiempo de vida visualice solo la escara(BZ,Core) poniendo el valor de los nodos sanos a -2
  if (key == 'e'){
    escara_case = !escara_case;
    if (escara_case)
      println("Grabado Ensight Tiempo de Vida solo nodos BZ y Core ");
    else
      println("Grabado Ensight Tiempo de Vida nodos Completos ");
  }

  //  Activamos la variable para que el grabado de .ens de mapas de activación visualice solo la escara(BZ,Core) poniendo el valor de los nodos sanos a -2
  if (key == 'z'){
    escara_caseA = !escara_caseA;
    if (escara_caseA)
      println("Grabado Ensight Mapa de Activación solo nodos BZ y Core ");
    else
      println("Grabado Ensight Mapa de Activación nodos Completos ");
  }
  //  Activamos la variable para que el grabado de .ens de mapas de activación visualice solo los nodos Mid poniendo el valor del resto de nodos a -2
  if (key == 'y'){
    mid_A = !mid_A;
    if (mid_A)
      println("Grabado Ensight Mapa de Activación solo nodos Mid ");
    else
      println("Grabado Ensight Mapa de Activación nodos Completos ");
  }

  //  0 = estado-celda ; 1 = tvida ; 2 = apd ; 3 = di; 4 = vel del frente; 5 = frente activ y frente desactiv
  if (key == 'm'){ // modo de visualizacion: tvida vs estado vs apd vs di
    caseParams.vmode++;
    if(caseParams.vmode>6)
      caseParams.vmode = 0;
  }

  // Visualiza -> 0 = Sana, 1 = BZ, 2 = BZ+Sana
  if (key == 't'){ // modo de visualizacion de tipo de células
    caseParams.tmode++;
    if(caseParams.tmode>2)
      caseParams.tmode = 0;
    println("Visualizando t:"+caseParams.tmode);
  }

  if (key == 'i'){ //ACtivar/desactivar visualización nodos inactivos
      alphaInactive = !alphaInactive;
      println("Visualizando celdas inactivas: ", alphaInactive);
  }

  if (key == 'l')
   println("Number of active nodes: ",ac.Labp.size(), "t actual: ", tiempo_transcurrido);

  if (key == 'f')
   caseParams.show_frente = !caseParams.show_frente;

  if (key == 'g'){
    caseParams.rec_frame = !caseParams.rec_frame;
    println("Grabar frames: ", caseParams.rec_frame);
  }

  if (key == 'w')
   caseParams.hay_mesh = !caseParams.hay_mesh;

  if (key == 'r')
   caseParams.restsurf = !caseParams.restsurf;


}


// id_view: 0: general, 1 izq, 2 der
void draw_scene(int id_view)
{
   int escala;
   if (rec_case){
     generaEns();
     t_case++;
   }

  // Camara
  pushMatrix();


    switch(caso) {

      case VENT:
               if(!caseParams.multi_view){
                 strokeWeight(6);
                 escala = 5;
                 // Berruezo
                 scale(-1,1,1);
                 //Alejandro
                  //scale(1,-1,1);
                  rotateX(PI/2);
                  translate(-cas.centerMass.x,-cas.centerMass.y,-cas.centerMass.z);
                  cam.setDistance(125);

                  // Alejandro
                  //escala = 5;
                  //scale(-escala,escala,escala);
                  //rotateX(-PI/2);
                  //translate(-65,-65,-250);
                  //cam.setDistance(600);
               }
               else{
               if (id_view == 1){ // vista posterior // der
                 strokeWeight(1);
                 rotateY(PI);
                 scale(-1,1,1);
                 rotateX(PI/2);
                 translate(-cas.centerMass.x,-cas.centerMass.y,-cas.centerMass.z);
                }
                else {              // vista frontal // izq
                  strokeWeight(1);
                  scale(-1,1,1);
                  rotateX(PI/2);
                  translate(100-cas.centerMass.x,-cas.centerMass.y,-cas.centerMass.z);
                }
                cam.setDistance(190);
               }
        break;

      case BLOQUE_VTK:
        escala = round(0.55 / cas.cellsize_bloqueVTK);
        scale(escala,-escala,escala);
        translate(-cas.centerMass.x*1.5,-cas.centerMass.y,-cas.centerMass.z);
        if (cas.cellsize_bloqueVTK < 0.2)
          cam.setDistance(80);
        else if (cas.bpl_CasoBloqueY * cas.cellsize_bloqueVTK < 140)
          cam.setDistance(130); //prima100
        else
          cam.setDistance(180); //prima140
        break;
    }


    //Mesh endo
    if(caseParams.hay_mesh) {
        noStroke();
        fill(25,25,25,255);
        gfx.mesh(mesh1, true);
    }


    if (caseParams.grid_enable)
      grid.reset();


    // Grid: add: se insertan en el grid solo las visibles
    if (caseParams.show_frente)
      ac.draw_insert(caseParams.vmode, caseParams.show_graph);
    // Inicializamos número de celdas activas en cada draw fuera de la función draw_insert para que no dependa de la visualización
    ac.n_cells_updated = 0;
    if (caseParams.grid_enable)
      grid.draw(1);

    /* Para ver el camino más largo  */
    if (caseParams.show_path)
      ac.show_lpath();
  popMatrix();

   if (caseParams.show_cmap)
      draw_cmap();

   if (caseParams.rec_frame)
    saveFrame("video/videoFrames/simulacion_########.png");

}

void draw_cmap(){

  noStroke();
  //pushMatrix();
  cam.beginHUD(); // Función para que sea independiente de los movimientos y la vista de la cámara

  float r,g,b;

  if (caso == BLOQUE_VTK){
    strokeWeight(1);
    translate(800,160,0);
    scale(3,3,3);
  }
  else if (caso == VENT){
    strokeWeight(3);
    if(!caseParams.multi_view){
      translate(800,160,0);
      scale(3,3,3);
    }
    // Si multi view
    else{
      translate(800,160,0); //Independiente del ventriculo
      scale(3,3,3);
    }
  }
    for (float tv = caseParams.rango_visual; tv > 0; tv-=.5) {

      if(tv > caseParams.rango_visual/2) {
        r = map(tv, caseParams.rango_visual, caseParams.rango_visual/2, 255, 0);
        b = 0;
        g = map(tv, caseParams.rango_visual, caseParams.rango_visual/2, 0, 255);
       }
       else{
        r =  0;
        b = map(tv, caseParams.rango_visual/2, 0, 0,255);
        g = map(tv, caseParams.rango_visual/2, 0, 255, 0);
       }

       for (float x = 0; x < 4; x+=.5) {
           stroke(r,g, b);
           point(x, 100-100*tv/caseParams.rango_visual, 0);
       }
    }

    scale(0.5);
    textSize(10);
    fill(0, 0, 0);
    // Tiempo transcurrido
    if (caso == BLOQUE_VTK)
      text("t: "+str(tiempo_transcurrido),-10, -20);
    else
      text("t: "+str(tiempo_transcurrido),20, -20);
    //  0 = estado-celda ; 1 = tvida ; 2 = apd ; 3 = di; 4 = vel del frente
    if (caseParams.vmode == 0)
      text("Cell State",15, lado);
    else
    if (caseParams.vmode == 1)
      text("Life Time (ms)",15, lado);
    else
    if (caseParams.vmode == 2)
      text("APD (ms)",15, lado);
    else
    if (caseParams.vmode == 3)
      text("Diastolic \ninterval (ms)",15, lado);
    else
    if (caseParams.vmode == 4)
      text("Wave front \nspeed",15, lado);
    else
    if (caseParams.vmode == 5)
      text("Wave front \nactive-inactive",15, lado);
    else
    if (caseParams.vmode == 6)
      text("Activation period \nmap (ms)",15, lado);

    text(str(caseParams.rango_min),10, 2*lado);
    text(str(caseParams.rango_visual),10, 5);

    cam.endHUD(); // Fin independiente de cámara
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
////// FUNCIONES PARA GENERAR LOS ARCHIVOS DEL GRABADO DE LA ANIMACIÓN DE TIEMPOS DE VIDA EN .case ////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Función que borra carpeta para no mezclar archivos de anteriores simulaciones
boolean deleteDirectory(File directoryToBeDeleted) {
    File[] allContents = directoryToBeDeleted.listFiles();
    if (allContents != null) {
        for (File file : allContents) {
            deleteDirectory(file);
        }
    }
    return directoryToBeDeleted.delete();
}

// Archivo.geo
void generaGeo(String typeFile){

  // Comprobamos si carpeta base ya existe para borrarla y crearla de nuevo, para no mezclar archivos de anteriores simulaciones
  Path path = Paths.get(casePath+typeFile+"_"+paciente);
  if (Files.exists(path)){
    boolean deleted = deleteDirectory(path.toFile());
    println("  BORRADO DE CARPETA: ", typeFile+"_"+paciente, deleted);
  }
  // Se crea carpeta y archivo geo
  PrintWriter outputGeo;
  File geofile = new File(casePath+typeFile+"_"+paciente+"/Vent_"+paciente+".geo");
  println("  CARPETA CREADA: ", typeFile+"_"+paciente);
  outputGeo  = createWriter(geofile);

  // HEADERS
  outputGeo.println("Ensight Model Geometry File");
  outputGeo.println("\t\t\t\t\tSIMULATOR OUTPUT");
  outputGeo.println("node id given");
  outputGeo.println("element id given");
  outputGeo.println("part");
  outputGeo.println("\t\t 1");
  outputGeo.println("Model, Geometry          1");
  outputGeo.println("coordinates");
  // Nº de nodos
  outputGeo.println(str(ac.G.size()));
  outputGeo.flush(); // Writes the remaining data to the file

  // id's de nodos
  for (int i = 1 ; i <= ac.G.size(); i++)
    outputGeo.println(str(i));
  outputGeo.flush(); // Writes the remaining data to the file

  // coordenadas x de todos los nodos
  for (Node3 n: ac.G){
    outputGeo.println(str(n.pos.x));
  }
  outputGeo.flush();
  // coordenadas y de todos los nodos
  for (Node3 n: ac.G){
    outputGeo.println(str(n.pos.y));
  }
  outputGeo.flush();
  // coordenadas z de todos los nodos
  for (Node3 n: ac.G){
    outputGeo.println(str(n.pos.z));
  }
  outputGeo.flush();

  // Tipo de elementos
  outputGeo.println("tetra4");
  //outputGeo.println("hexa8");
  // Abrimos archivo txt con id's de nodos conectados a cada celda ne cada línea
  String[] lines = loadStrings(casePath+"Reader_VTK/cell_conex_nodos.txt");

  // Nº de elementos
  outputGeo.println("\t"+str(lines.length));

  // Id's elementos
  for (int i = 1 ; i <= lines.length; i++){
     outputGeo.println("\t\t"+str(i));
  }
  outputGeo.flush();

  // copiamos cada linea de nodos conectados a cada celda
  // Modificando el orden de los elementos voxels a orden de hexa8 (ya que ensight no acepta formato de elemento voxel)
  // Orden distinto si Ventrículo o Bloque vtk
  for (int i = 0 ; i < lines.length; i++){
     String[] tok = split(lines[i], ' ');
     String hexaline = "";
     // Si ventrículo
     if (caso == VENT)
       // Ventrículo Alejandro
       //hexaline = " "+str(int(tok[0])+1)+" "+str(int(tok[1])+1)+" "+str(int(tok[2])+1)+" "+str(int(tok[3])+1)+" "+str(int(tok[4])+1)+" "+str(int(tok[5])+1)+" "+str(int(tok[6])+1)+" "+str(int(tok[7])+1);
       // Ventrículos Berruezo Voxels
       //hexaline = " "+str(int(tok[0])+1)+" "+str(int(tok[1])+1)+" "+str(int(tok[3])+1)+" "+str(int(tok[2])+1)+" "+str(int(tok[4])+1)+" "+str(int(tok[5])+1)+" "+str(int(tok[7])+1)+" "+str(int(tok[6])+1);
       // Mesh Tetraedro
       hexaline = " "+str(int(tok[0])+1)+" "+str(int(tok[1])+1)+" "+str(int(tok[2])+1)+" "+str(int(tok[3])+1);
     // Si Bloque vtk
     else
       hexaline = " "+str(int(tok[0])+1)+" "+str(int(tok[1])+1)+" "+str(int(tok[2])+1)+" "+str(int(tok[3])+1)+" "+str(int(tok[4])+1)+" "+str(int(tok[5])+1)+" "+str(int(tok[6])+1)+" "+str(int(tok[7])+1);
     //String hexaline = " "+tok[0]+" "+tok[1]+" "+tok[2]+" "+tok[3]+" "+tok[4]+" "+tok[5]+" "+tok[6]+" "+tok[7];
     outputGeo.println("\t\t\t"+hexaline);
  }

  outputGeo.flush();

  outputGeo.close(); // Finishes the file
  println(" Archivo .geo creado");
}

// Archivo .case
void generaCase(int steps){

  PrintWriter outputCase;
  File casefile = new File(casePath+"Case_"+paciente+"/Vent_"+paciente+".case");
  outputCase  = createWriter(casefile);

  // HEADERS
  outputCase.println("FORMAT");
  outputCase.println("type:                   ensight gold");
  outputCase.println("GEOMETRY");
  outputCase.println("model:                              Vent_"+paciente+".geo");
  outputCase.println("");
  outputCase.println("VARIABLE");
  outputCase.println("");
  outputCase.println("scalar per node:           Life_Time                  Vent_"+paciente+"_00000****.ens");
  outputCase.println("");
  outputCase.println("TIME");
  outputCase.println("");
  outputCase.println("time set:               1");
  outputCase.println("number of steps:           "+steps);
  outputCase.println("filename start number:  0");
  outputCase.println("filename increment:     1");
  outputCase.println("time values:");
  for (int i = 0; i <= steps; i+=8){
    // Guardamos 8 valores de tiempos por línea
    String lineStep = "";
    for (int j = 0; j < 8; j++){
      float t_line = float(t_caseIni+i+j);
      if (t_line < t_caseIni + steps)
        lineStep += "         "+str(t_line);
    }
    outputCase.println(lineStep);
  }
  outputCase.flush(); // Writes the remaining data to the file

  outputCase.close(); // Finishes the file
  println(" Archivo .case creado");
}

// Archivo .ens
void generaEns(){
  // Formateamos el número del contador de steps para que añada ceros a la izquierda, rellenando hasta 10 enteros y devuelva un string formateado
  Formatter fmt = new Formatter();
  fmt.format("%09d",count_steps);

  PrintWriter outputEns;
  File ensfile = new File(casePath+"Case_"+paciente+"/Vent_"+paciente+"_"+fmt+".ens");
  outputEns  = createWriter(ensfile);

  count_steps++; // Incrementamos contador para nombre de siguiente archivo .ens

  // HEADERS
  outputEns.println("Ensight Model Post Process. Activation Time");
  outputEns.println("part");
  outputEns.println("         1");
  outputEns.println("coordinates");
  outputEns.flush(); // Writes the remaining data to the file

  // Tiempo de vida de cada nodo
  for (Node3 n: ac.G){
    //float valor = 0.0;
    float valor = n.t_proxima_desactivacion - tiempo_transcurrido;
    // Si el nodo es core tendrá APD 0 y le ponemos valor de tvida a 0. Si queremos no visualizarlos en Paraview con un threshold
    // podemos modificar el valor a -1
    if (n.apd == 0.0)
      valor = 0.0;
    // SI la celda está desactivada t_proxima_desactivacion es infinito por tanto si valores mayor que 1000 ponemos tvida a 0
    else if (valor > 1000)
      valor = 0.0;
    // Si nodo Endo o Epi valor -2 para quedarnos solo con MID con threshold
    //if (n.endo2Epi == 0 || n.endo2Epi == 2){
      //valor = -2;
    //}
    //else
      //valor = n.tvida;
      //valor = n.tvida/n.apd;
      //valor = n.apd;
      //valor = 350*n.tvida/n.apd;
      //valor = n.t_proxima_desactivacion - tiempo_transcurrido;

    // Para visualizar en Paraview solo la activación de BZ y Core(desactivado fijo) ponemos a valor -1 el Core y a -2 todas las celdas sanas para no visualizarlas con threshold
    if (escara_case){
      // Si Core
      if (n.tipo == 2)
        valor = -1;
      // Si Sanas
      else if (n.tipo == 0)
        valor = -2;
    }
    outputEns.println(str(valor));
  }

  outputEns.flush(); // Writes the remaining data to the file
  outputEns.close(); // Finishes the file
  //println(" Archivo Vent_"+paciente+"_"+fmt+".ens creado");

  fmt.close();
}


////////////////////////////////////////////////////////////////////////////////////////////////
////// FUNCIONES PARA GENERAR LOS ARCHIVOS DEL GRABADO DE LOS MAPAS DE ACTIVACIÓN .case ////////
////////////////////////////////////////////////////////////////////////////////////////////////

// Archivo .case Mapas de tiempos, dirección y velocidad de activación en cada nodo
void generaCaseA_Map(){

  PrintWriter outputCase;
  File casefile = new File(casePath+"A_Map_"+paciente+"/VentA_Map_"+paciente+".case");
  outputCase  = createWriter(casefile);

  // HEADERS
  outputCase.println("FORMAT");
  outputCase.println("type:                   ensight gold");
  outputCase.println("GEOMETRY");
  outputCase.println("model:                              Vent_"+paciente+".geo");
  outputCase.println("");
  outputCase.println("VARIABLE");
  outputCase.println("");
  outputCase.println("scalar per node:           Activation_Map                  VentA_Map_"+paciente+"_00000****.ens");
  //outputCase.println("scalar per node:           Activation_Vel                  VentA_Vel_"+paciente+"_00000****.vel");
  outputCase.println("scalar per node:           Activation_APD                  VentA_APD_"+paciente+"_00000****.apd");
  outputCase.println("");
  outputCase.println("TIME");
  outputCase.println("");
  outputCase.println("time set:               1");
  outputCase.println("number of steps:           "+numBeat);
  outputCase.println("filename start number:  0");
  outputCase.println("filename increment:     1");
  outputCase.println("time values:");
  println("t Beats antes en case", t_beats);
  for (int i = 0; i < t_beats.size(); i+=8){
    // Guardamos la lista de valores de tiempo de activación de cada estímulo
    String lineStep = "";
    float t_line = 0.0;
    for (int j = 0; j < 8; j++){
      if (i+j < t_beats.size()){
        t_line = t_beats.get(i+j);
        lineStep += "         "+str(t_line);
      }
    }
    outputCase.println(lineStep);
  }
  outputCase.flush(); // Writes the remaining data to the file

  outputCase.close(); // Finishes the file
  println(" Archivo Activation Map .case creado");
}


// Archivo .ens  Mapas de activación y le pasamos la lista que debe de guardar por si es primera, segunda o reentrada
void generaEnsA_Map(FloatList a_times, FloatList a_vel){
  // Formateamos el número del contador de estímulos para que añada ceros a la izquierda, rellenando hasta 10 enteros y devuelva un string formateado
  Formatter fmt = new Formatter();
  fmt.format("%09d",numBeat);
  // Archivo .ens que guarda el mapa de tiempos de activación
  PrintWriter outputEns;
  File ensfile = new File(casePath+"A_Map_"+paciente+"/VentA_Map_"+paciente+"_"+fmt+".ens");
  outputEns  = createWriter(ensfile);
  // Archivo .vel que guarda las velocidades de activación
  PrintWriter outputVel;
  //File velfile = new File(casePath+"A_Map_"+paciente+"/VentA_Vel_"+paciente+"_"+fmt+".vel");
  File velfile = new File(casePath+"A_Map_"+paciente+"/VentA_APD_"+paciente+"_"+fmt+".apd");
  outputVel  = createWriter(velfile);

  numBeat++; // Incrementamos contador para nombre de siguiente archivo .ens .dir

  // HEADERS
  outputEns.println("Ensight Model Post Process. Activation Time");
  outputEns.println("part");
  outputEns.println("         1");
  outputEns.println("coordinates");
  outputEns.flush(); // Writes the remaining data to the file

  // HEADERS
  outputVel.println("Ensight Model Post Process. Velocity");
  outputVel.println("part");
  outputVel.println("         1");
  outputVel.println("coordinates");
  outputVel.flush(); // Writes the remaining data to the file


  // Guardamos en cada línea de los archivos el tiempo y velocidad de activación de cada nodo en estímulo actual, guardado en lista de tiempos a lo largo del estímulo
  float valor = 0.0;
  float valorVel = 0.0;

  for (int i=0; i < a_times.size(); i++){
    valor = a_times.get(i);
    valorVel = a_vel.get(i);
    // Si solo queremos valores de escara, en los nodos sanos reescribimos valor de t activación y velocidad a -2, para después hacer threshold y descartarlos
    if (escara_caseA && !mid_A){
      Node3 n = ac.G.get(i);
      // Si sanas
      if (n.tipo == 0){
        valor = -2;
        valorVel = -2;
      }
    }
    // Si solo queremos valores de nodos mid, en los nodos endo y epi reescribimos valor de t activación y velocidad a -2, para después hacer threshold y descartarlos
    else if (mid_A && !escara_caseA){
      Node3 n = ac.G.get(i);
      // Si nodo Endo o Epi
      if (n.endo2Epi == 0 || n.endo2Epi == 2){
        valor = -2;
        valorVel = -2;
      }
    }
    // Si solo queremos valores de nodos mid pero solo de BZ, en los nodos endo y epi y que pertenezcan a nodos sanos, reescribimos valor de t activación y velocidad a -2, para después hacer threshold y descartarlos
    else if (mid_A && escara_caseA){
      Node3 n = ac.G.get(i);
      // Si  Sana descartamos
      if (n.tipo == 0){
        valor = -2;
        valorVel = -2;
      }
      // Si no, si BZ pero Endo o Epi descartamos
      else if (n.endo2Epi == 0 || n.endo2Epi == 2){
        valor = -2;
        valorVel = -2;
      }
    }
    outputEns.println(str(valor));
    outputVel.println(str(valorVel));
  }
  outputEns.flush(); // Writes the remaining data to the file
  outputEns.close(); // Finishes the file
  println(" Archivo VentA_Map_"+paciente+"_"+fmt+".ens creado");

  outputVel.flush(); // Writes the remaining data to the file
  outputVel.close(); // Finishes the file
  println(" Archivo VentA_Map_"+paciente+"_"+fmt+".vel creado");

  fmt.close();
}
