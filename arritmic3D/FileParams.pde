class FileParams{
    String    model_file;
    Boolean   hay_mesh;
    int       pinta_cada_n;
    int       id_extraI;
    Boolean   active_MultiIDs_extraI; // Variable para activar opción de activación de nodos definidos en txt
    // Nombre de archivo txt para activación de nodos definidos, que contiene en cada línea: id nodo a activar, delay de tiempo de activación y vector XYZ de dirección de propagación (Todo separado por espacios)
    IntList   ids_extraIMulti = new IntList(); // Lista con id's de nodos iniciales de activación Multi
    FloatList delays_extraIMulti = new FloatList(); // Lista con delays de nodos iniciales de activación Multi
    ArrayList<PVector> dirProp_extraIMulti = new ArrayList(); // Lista con vectores XYZ de dirección de propagación de de nodos iniciales de activación Multi
    String    fileName_MultiIDs_extraI; 
    float     voxel_size;
    float     tam_frente;
    Boolean   grid_enable;
    Boolean   multi_view;
    Boolean   show_cmap;
    float     dt;      // milisegundos
    int       vmode;      //  0 = estado-celda ; 1 = tvida ; 2 = apd ; 3 = di; 4 = vel del frente
    int       tmode;      // Visualiza -> 0 = Sana, 1 = BZ, 2 = BZ+Sana
    int       fps;
    Boolean   show_path;
    Boolean   show_frente;
    Boolean   show_graph;
    Boolean   rec_frame;
    Boolean   show_gui;
    int       rango_visual;
    int       rango_min;
    float     radio_cateter; // Radio del cateter de pacing para definir num de nodos iniciales de estímulos
    float     min_pot_act; // Potencial de activación mínimo para que una celda se active
    
    float     di_Sana; // Intervalo diastólico célula sana  DI ->200 (APD - 285 CV - 0.67)
    float     di_BZ ;  // Intervalo diastólico célula BZ. Le reducimos de la sana, ya que el APD es más largo

    float     stimFrecS1; // Frecuencia de estimulo S1
    float     stimFrecS2; // Frecuencia de estimulo S2 S3
    int       nStimsS1;
    int       nStimsS2;
    
    float     apd_varBZ; // Aumentamos un 20% el APD de la BZ
    float     vel_suelo; // Velocidad suelo para uqe no baje por debajo de tiempos no correctos
    float     apd_memory; // % Memoria AOD
    float     cv_memory;  // % Memoria CV
    float     reductCVtr; // 0.38% de la CV longitudinal a aplicar en transversal
    float     apd_isot_ef; //Efecto electrotónico. % que afecta
    
    PVector   direccion_fibras_bloque; 
    
    // Multi simulación (Listas con los distintos parámetros a simular en una misma ejecución)
    boolean       multiSim;
    FloatList     id_extraIMulti;
    FloatList     stimFrecS1Multi; // Frecuencia de estimulo S1
    FloatList     stimFrecS2Multi; // Frecuencia de estimulo S2 S3
    FloatList     nStimsS1Multi;   // Nª de estímulos de S1
    FloatList     nStimsS2Multi;   // Nª de estímulos de S2
    FloatList     cv_memoryMulti;  // % Memoria CV
    FloatList     apd_isot_efMulti;//Efecto electrotónico. % que afecta


    
    FileParams(String caseFile){
      
       BufferedReader reader = createReader(caseFile+"params.dat");
       println("Parametros cargados: ");    
        try {
          
          model_file       =  parse_param(reader.readLine());
          hay_mesh         =  boolean(parse_param(reader.readLine()));
          grid_enable      =  boolean(parse_param(reader.readLine()));
          pinta_cada_n     =  int(parse_param(reader.readLine()));
          id_extraI        =  int(parse_param(reader.readLine()));
          active_MultiIDs_extraI =  boolean(parse_param(reader.readLine()));
          fileName_MultiIDs_extraI = parse_param(reader.readLine());
          voxel_size       =  float(parse_param(reader.readLine()));
          tam_frente       =  float(parse_param(reader.readLine()));
          multi_view       =  boolean(parse_param(reader.readLine()));
          show_cmap        =  boolean(parse_param(reader.readLine()));
          dt               =  float(parse_param(reader.readLine()));
          vmode            =  int(parse_param(reader.readLine()));
          tmode            =  int(parse_param(reader.readLine()));
          fps              =  int(parse_param(reader.readLine()));
          show_path        =  boolean(parse_param(reader.readLine()));
          show_frente      =  boolean(parse_param(reader.readLine()));
          show_graph       =  boolean(parse_param(reader.readLine()));
          rec_frame        =  boolean(parse_param(reader.readLine()));
          show_gui         =  boolean(parse_param(reader.readLine()));
          rango_visual     =  int(parse_param(reader.readLine()));
          rango_min        =  int(parse_param(reader.readLine()));
          radio_cateter    =  float(parse_param(reader.readLine()));
          min_pot_act      = float(parse_param(reader.readLine()));
          di_Sana          =  float(parse_param(reader.readLine()));
          di_BZ            =  float(parse_param(reader.readLine()));
          stimFrecS1       =  float(parse_param(reader.readLine()));
          stimFrecS2       =  float(parse_param(reader.readLine()));
          nStimsS1         =  int(parse_param(reader.readLine()));
          nStimsS2         =  int(parse_param(reader.readLine()));
          apd_memory       =  float(parse_param(reader.readLine()));
          cv_memory        =  float(parse_param(reader.readLine()));
          reductCVtr       =  float(parse_param(reader.readLine()));
          apd_isot_ef      =  float(parse_param(reader.readLine()));
          direccion_fibras_bloque = parse_PVector(parse_param(reader.readLine()));
          
          //Multi Simulación
          multiSim         =  boolean(parse_param(reader.readLine()));
          id_extraIMulti   =  parse_FloatList(parse_param(reader.readLine()));
          stimFrecS1Multi  =  parse_FloatList(parse_param(reader.readLine()));
          stimFrecS2Multi  =  parse_FloatList(parse_param(reader.readLine()));
          nStimsS1Multi    =  parse_FloatList(parse_param(reader.readLine()));
          nStimsS2Multi    =  parse_FloatList(parse_param(reader.readLine()));
          cv_memoryMulti   =  parse_FloatList(parse_param(reader.readLine()));
          apd_isot_efMulti =  parse_FloatList(parse_param(reader.readLine()));
          
          reader.close();
          println("Leido Fichero de parametros: ", caseFile);
         }
         catch (IOException e) {
          e.printStackTrace();
         }
         // Si variable active_MultiIDs_extraI a true y consta el nombre del archivo, leemos el archivo txt para obtener listas de ids, delays y direccion de propagación de los nodos del frente a activar
         if (active_MultiIDs_extraI && fileName_MultiIDs_extraI.trim().length() != 0)
           readInitNodesMulti(caseFile);
         // Si variable a true pero no han escrito el nombre del archivo ponemos la variable a false para que no se ejecute la activación del frente
         else if (active_MultiIDs_extraI)
           active_MultiIDs_extraI = false;
           
         
            
    }
    
    void readInitNodesMulti(String caseFile){
      print("Path: ", caseFile+fileName_MultiIDs_extraI);
      BufferedReader readerInitNodesMulti = createReader(caseFile+fileName_MultiIDs_extraI);
      try {
         String line;
         while ((line = readerInitNodesMulti.readLine()) != null) {
          String[] values = line.split("\\s+");
          ids_extraIMulti.append(int(values[0]));
          delays_extraIMulti.append(float(values[1]));
          dirProp_extraIMulti.add(new PVector(float(values[2]),float(values[3]),float(values[4])));
         }
         println("Datos fichero txt IdsExtraMulti cargado");
      }
      catch (IOException e) {
          e.printStackTrace();
      }
    }
    
    String parse_param(String line){
      
      println("  |-> "+line);
      if (line != null){ 
            String[] params = split(line, ':');
            if (params.length == 2)
              return (params[1].trim());
            else
              return null;
      }
      return null;
    }
    
    PVector parse_PVector(String line){
      
      println("  |-> "+line);
      if (line != null){ 
            String[] params = split(line, ':');
            String svec = params[0];
            String[] vec = split(svec, ','); 
            return new PVector(float(vec[0]), float(vec[1]), float(vec[2]));
      }
      return null;
    }
    
    FloatList parse_FloatList(String line){
      
      println("  |-> "+line);
      if (line != null){ 
            FloatList values = new FloatList();
            String[] params = split(line, ':');
            String svec = params[0];
            String[] vec = split(svec, ','); 
            for (int i = 0; i < vec.length; i++)
              values.append(float(vec[i]));
            return values;
      }
      return null;
    }
    
    IntList parse_IntList(String line){
      
      println("  |-> "+line);
      if (line != null){
            IntList values = new IntList();
            String[] params = split(line, ':');
            String svec = params[0];
            String[] vec = split(svec, ','); 
            for (int i = 0; i < vec.length; i++)
              values.append(int(vec[i]));
            return values;
      }
      return null;
    }
}
