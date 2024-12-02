final int MINIMO = 0;
final int NODO_FANTASMA = 1;
final int LEAST_SQUARES = 2;

class Node3{
  int                 id, estado;
  PVector             pos;
  int                 tipo;
  int                 endo2Epi;
  int                 beat; // último latido que lo activó.
  float               grayLevel, cv, apd, tvida, start_time, end_time, di, kapd_v;
  float               cv_real;
  PVector             indices;
  PVector             orientacion;
  float               reductCV;
  float               lpath, derror,terror; //longitud del path recorrido y error respecto al foco (dst-linea recta)

  // Vecinos
  ArrayList<Node3>    Lv;  // Vecinos
  FloatList           Ldv, Lcv;     // Distancia a sus vecinos,  vel de cond. indep. para cada vecino 
  ArrayList<PVector>  Lprv; // Posición relativa al vecino  
  
  // Fuente de activación
  int                 tipo_propagacion;
  PVector             foco_activador;
  PVector             suma_activadores;
  PVector             normal_frente;
  float               tiempo_activador;
  float               suma_tiempos_activadores;
  int                 n_activadores;
  float               potencial;
  float               min_potencial;

  float               t_proxima_activacion; // tiempo absoluto de próxima activación
  float               t_proxima_desactivacion; // tiempo absoluto de próxima desactivación
  float               t_proximo_evento; // tiempo del proximo evento
  boolean             estimulo_externo;

  Evento              sig_evento;

  Node3               padre;
  Boolean             esperando;
  float               last_start_time;
  float               periodo_activacion;
  float               safety_factor;
  float               current_act_beat;
  float               factorCorrecCVBZ = 1;
  float               factorCorrecAPD = 1;
  
  
  Node3(int _id, PVector _pos, float _grLevel, int _est, PVector _or, PVector _ind,int _tipo, int _endo2Epi) {
    
    this.id = _id;
    this.pos = _pos;
    this.tipo = _tipo;
    this.endo2Epi = _endo2Epi;
    this.grayLevel = _grLevel;
    this.indices = _ind;
    this.orientacion = _or;
    this.orientacion.normalize();
    this.estado = _est;

    // Vecinos: ids, vectores, distancias, cv's
    this.Lv = new ArrayList<Node3>();
    this.Lprv = new ArrayList<PVector>();
    this.Ldv = new FloatList();
    this.Lcv = new FloatList();
    
    reset();
  }
  
  void reset(){
    t_proxima_activacion = INFINITO;
    desactivar();

    this.reductCV = caseParams.reductCVtr; // % de la CV longitudinal a aplicar en transversal
    this.beat = -1; 
    
    this.current_act_beat = -1; // Inicializamos el beat de activación a -1
    this.cv_real = 0; // Inicializamos a 0 la cv real que se calculará después aplicando la orientación de fibras y la velocidad respecto al frente o al origen 
  

    this.di = caseParams.di_Sana;
    if(this.tipo == 1)
      this.di = caseParams.di_BZ;
      
    this.tvida = 0; // Tiempo acumulado de vida desde que se activa la célula
    this.end_time = INFINITO; // Tiempo en el cual se desactiva la célula
    this.start_time = INFINITO; // Tiempo en el cual se ha activado la célula
    this.t_proxima_activacion = INFINITO; // Tiempo en el cual se activará la célula
    this.t_proxima_desactivacion = INFINITO; // Tiempo en el cual se desactivará la célula
    this.t_proximo_evento = INFINITO;
    this.sig_evento = null;
    
    this.estimulo_externo = false;
    
    // Obtenemos APD interpolado de la curva de restitución APD
    // En células sanas
    if (tipo == 0){
      // Si Epi
      if (endo2Epi == 2 )
        this.apd = cas.spline_Epiapd.interpolate(di)*factorCorrecAPD;
      // Si M
      else if (endo2Epi == 1 )
        this.apd = cas.spline_Mapd.interpolate(di)*factorCorrecAPD; 
      // Si Endo
      else 
        this.apd = cas.spline_Endoapd.interpolate(di)*factorCorrecAPD; 
    }
    // Si BZ
    else if (tipo == 1){
      // Si Epi
      if (endo2Epi == 2 )
        this.apd = cas.spline_Epiapd_BZ.interpolate(di)*factorCorrecAPD;
      // Si M
      else if (endo2Epi == 1 )
        this.apd = cas.spline_Mapd_BZ.interpolate(di)*factorCorrecAPD; 
      // Si Endo
      else 
        this.apd = cas.spline_Endoapd_BZ.interpolate(di)*factorCorrecAPD; 
    }
    // Si Core ponemos el valor de apd a 0 para que al generar los archivos case que usan todos los nodos, el Core esté desactivado a 0 y no falle al acceder al valor apd 
    else if (tipo == 2)
      this.apd = 0.0;

    this.tipo_propagacion = NODO_FANTASMA; // MINIMO | NODO_FANTASMA | LEAST_SQUARES

    this.suma_activadores = new PVector();
    this.suma_tiempos_activadores = -INFINITO;
    this.foco_activador = new PVector();
    this.tiempo_activador = -INFINITO;
    this.n_activadores = 0;
    this.normal_frente = new PVector();
    this.n_activadores = 0;
    this.min_potencial = caseParams.min_pot_act;
    
    // Calculamos la velocidad de la conducción para células sanas
    if (tipo == 0){
      // Si Epi
      if (endo2Epi == 2 )
        cv = cas.spline_Epicv.interpolate(di);
      // Si M
      else if (endo2Epi == 1 )
        cv = cas.spline_Mcv.interpolate(di); 
      // Si Endo
      else 
        cv = cas.spline_Endocv.interpolate(di); 
    }  
    // Si célula tipo BZ calculamos la velocidad de la conducción 
    else if (tipo == 1) {
      // Si Epi
      if (endo2Epi == 2 )
        cv = (cas.spline_Epicv_BZ.interpolate(di))*factorCorrecCVBZ;
      // Si M
      else if (endo2Epi == 1 )
        cv = (cas.spline_Mcv_BZ.interpolate(di))*factorCorrecCVBZ; 
      // Si Endo
      else 
        cv = (cas.spline_Endocv_BZ.interpolate(di))*factorCorrecCVBZ; 
    }
    //Peso del apd de los vecinos en el propio apd
    this.kapd_v = apd_isot_ef; 
    this.lpath = 0;
    
    this.esperando = false;
   
  }

  
  Evento siguienteEvento(){
    if(sig_evento == null)
      sig_evento = new Evento();

    sig_evento.id = id;
    sig_evento.t = t_proximo_evento;
    sig_evento.st = estado;

    return sig_evento;
  }
  
  
  float calculaCV(PVector d) {
    // Core no conduce
    if(tipo == 2)
      return 0.0;
      
    // BZ Isotrópico
    if (tipo == 1)
      return cv;
      
    // Sanas anisotrópico
    // cv_t = transversal; cv_l = longitudinal
    float cv_t,cv_l;
    if(d.mag() == 0.0 || orientacion.mag() == 0.0) {
      cv_l = 1.0;
      cv_t = 0.0;
    }else{
      cv_l = abs(d.dot(orientacion)/d.mag());
      cv_t = sqrt(1.0 - cv_l*cv_l);
    }
    // Posición relativa en el espacio escalado según orientación de fibras
    // Al ser más lento en la dirección transversal, el espacio es estira
    // en esa dirección y los puntos están más lejos.
    PVector p = new PVector(cv_l,cv_t/reductCV);
    return cv/p.mag();
  }




  /**
   * Propaga la activación desde un nodo vecino
   */
  Evento propaga_activacion(Node3 origen,
                            float len_path,
                            int in_beat){

    Evento ev = null;
    PVector new_suma_activadores;
    float new_suma_tiempos;

    if (estado == 2) {
      // Si me está llegando la activación durante la fase refractaria hay que ignorarlo.
      // Pero si estoy al final de la fase refractaria, entonces hay que procesarlo aunque no me active
      // Definimos el final de la fase refractaria en función de la distancia del nodo. El nodo que
      // la activa no puede haberse activado antes del tiempo de desactivación menos el tiempo
      // máximo que puede tardar. Tomo como cv la más lenta posible, cv*reductCV
      float min_t_activador = this.t_proxima_desactivacion - PVector.dist(this.pos,origen.pos)/( origen.cv*origen.reductCV );

      // Si nos están activando desde un tiempo anterior asumimos que estamos
      // en mitad de la fase refractaria  =>  Lo ignoramos
      // TODO: Si estamos próximos a la desactivación el APD podría alargarse un poco
      if (origen.start_time <= min_t_activador)
        return ev;

    }

    // Calculamos la activación directa, desde el origen
    PVector n_orig = PVector.sub(this.pos,origen.pos);
    float vel_calc_orig = origen.calculaCV(n_orig);
    float d_orig = PVector.dist(this.pos,origen.pos);
    float new_t_activacion_orig = origen.start_time + d_orig / vel_calc_orig;


    // Suponemos que hay activación directa a través del grafo
    // Es la definitiva si tipo_propagacion == MINIMO
    boolean act_orig = true;
    PVector new_n = n_orig;
    float new_vel_calc = vel_calc_orig;
    float new_d = d_orig;
    float new_t_activacion = new_t_activacion_orig;
    PVector new_foco_activador = origen.pos;
    float new_tiempo_activador = origen.start_time;
    new_suma_tiempos = origen.start_time;
    new_suma_activadores = origen.pos;


    if(this.tipo_propagacion == NODO_FANTASMA) {
      // Calculamos un candidato a activador
      // Es el promedio de todos los activadores hasta el momento
      // Si no hay ninguno (n_activadores == 0) entonces es el actual
      if(this.n_activadores > 0) {
        new_suma_activadores = PVector.add(origen.pos,suma_activadores); // Suma. Para promedio hay que dividir por n
        new_suma_tiempos = suma_tiempos_activadores + origen.start_time;
      }
      PVector foco_fantasma = PVector.mult(new_suma_activadores, 1.0 / (this.n_activadores + 1)); // Promedio de posiciones. Nodo fantasma
      float tiempo_activador_fantasma = new_suma_tiempos / (this.n_activadores + 1); // Promedio de tiempos. Tiempo de activación del nodo fantasma


      // El tiempo activador es cuando se activa el foco activador.
      // Ahora calculamos el frente de onda y el tiempo de activación
      PVector n_foco_fantasma = PVector.sub(this.pos,foco_fantasma);
      float vel_calc_fantasma = origen.calculaCV(n_foco_fantasma);
      float d_fantasma = PVector.dist(this.pos,foco_fantasma);
      float t_activacion_fantasma = tiempo_activador_fantasma + d_fantasma / vel_calc_fantasma;

      // Nos quedamos con el promedio si es más temprana que el origen actual
      if (t_activacion_fantasma < new_t_activacion_orig) {
        new_n = n_foco_fantasma;
        new_vel_calc = vel_calc_fantasma;
        new_d = d_fantasma;
        new_t_activacion = t_activacion_fantasma;
        new_foco_activador = foco_fantasma;
        new_tiempo_activador = tiempo_activador_fantasma;
        act_orig = false;
      }

      // Si la activación sale en el pasado, la traemos al presente
      if(new_t_activacion < tiempo_transcurrido){
        new_t_activacion = tiempo_transcurrido;
      }
    }

    if(new_t_activacion <= t_proxima_activacion) {
      // Sólo lo validamos si el tiempo va a ser anterior
      // No comprobamos aún si la celda está activa
      // Aunque un vecino no sea capaz de activarme, su posición y tiempo
      // de activación son necesarios para calcular el frente.
      if(this.tipo_propagacion == NODO_FANTASMA) {
        suma_tiempos_activadores = new_suma_tiempos;
        suma_activadores = new_suma_activadores;
        this.n_activadores++;
      }
      foco_activador = new_foco_activador;
      tiempo_activador = new_tiempo_activador;
      normal_frente = new_n;
      normal_frente.normalize();
      float t_espera = new_t_activacion -  tiempo_transcurrido;
      ev = this.en_espera(t_espera,len_path,origen,in_beat,false);
    }
    if (ev != null)
      cv_real = new_vel_calc;
    return ev;
  }

  /**
   *Activa el modo de espera: la celda espera t_espera antes de activarse
   */
  Evento en_espera(float t_espera,float len_path, Node3 pdr,int in_beat,boolean estimulo_ext){
    Evento ev = null;
    if (pdr != null && safety_factor_active)
        if (pdr.safety_factor < this.safety_factor)
          return ev;

    boolean updated = false;

    float nuevo_t_activacion = tiempo_transcurrido + t_espera;

    // Sólo si lo activa otro vecino antes modificamos t espera
    if (nuevo_t_activacion < t_proxima_activacion){
      if(estado < 1){ // Estamos inactivos o en espera
        t_proxima_activacion = nuevo_t_activacion;
        t_proximo_evento = t_proxima_activacion;
        estado = 0;
        ev = siguienteEvento();
        // Pasamos el beat de activación del padre inicial a cada nodo que se active, siempre que no sea el padre inicial que genera el beat (init_nodes)
        if (!ac.init_nodes.hasValue(id))
          current_act_beat = pdr.current_act_beat;  
        updated = true;
        
        if(in_beat < beat  && ac.detecta_reentradas) // Añadir gestión de la reentrada más larga
          println("            REENTRADA?!?!?!  latido(AC) =", ac.num_beat, " in =",in_beat, " cur =",beat);
        
        
      } else {
        // Estamos activos. Hay que comprobar si ya está repolarizado
        // En este caso, no se genera el evento, que se generará al desactivar
        if(nuevo_t_activacion > t_proxima_desactivacion) {
          t_proxima_activacion = nuevo_t_activacion;
          updated = true;
          if(in_beat < beat && ac.detecta_reentradas)
            println("     --     REENTRADA?!?!?!  latido(AC) =", ac.num_beat, " in =",in_beat, " cur =",beat);
        }
      }
    }

    if(updated){ // Se ha actualizado. Cambiamos el padre para el camino
      estimulo_externo = estimulo_ext;
      padre = pdr;
      lpath = len_path;
      if(estimulo_externo)
        potencial += 1.0;
    }

    return ev;
  }
  
  
  Evento desactivar(){
    
    Evento ev = null;
      tvida = 0;
      last_start_time = start_time;
      start_time = INFINITO;
      end_time = tiempo_transcurrido;
      suma_tiempos_activadores = 0.0;
      tiempo_activador = 0.0;
      suma_activadores = new PVector(0,0,0);
      foco_activador = new PVector(0,0,0);
      normal_frente = new PVector(0,0,0);
      n_activadores = 0;
      potencial = 0;

      if(t_proxima_activacion < INFINITO){
        estado = 0; // Si ya hay tiempo de activación, pasamos a "en espera".
        t_proximo_evento = t_proxima_activacion;
        ev = siguienteEvento();
      } 
      else {
        estado = -1;
        t_proxima_desactivacion = INFINITO;
        t_proximo_evento = INFINITO;
        lpath = 0;
        derror = 0;
        terror = 0;
      }
     beat++;

    return ev;
  }
  
  
  void calcularActivacion() {
    // En la primera activación no calculamos el DI, ni APD memory porque no hay datos previos
    // Cuando se lance un estímulo y la célula ya se haya activado una vez, entonces
    // se calcula, usando información de la activación anterior.
    
    // Si segunda o siguientes activaciones ya calculamos media vecinos y memoria APD
    if(end_time != INFINITO){ // Primera activación no entramos
      di = tiempo_transcurrido - end_time;
      
      // Calcular el nuevo apd en funcion del di
      float new_apd = 0;
      // Si células sanas
      if (tipo == 0){
        // Si Epi
        if (endo2Epi == 2 )
          new_apd = cas.spline_Epiapd.interpolate(di)*factorCorrecAPD;
        // Si M
        else if (endo2Epi == 1)
          new_apd = cas.spline_Mapd.interpolate(di)*factorCorrecAPD; 
        // Si Endo
        else {
          new_apd = cas.spline_Endoapd.interpolate(di)*factorCorrecAPD; 
        }
      }
      // Si BZ
      else if (tipo == 1) {
        // Si Epi
        if (endo2Epi == 2 )
          new_apd = cas.spline_Epiapd_BZ.interpolate(di)*factorCorrecAPD;
        // Si M
        else if (endo2Epi == 1)
          new_apd = cas.spline_Mapd_BZ.interpolate(di)*factorCorrecAPD; 
        // Si Endo
        else 
          new_apd = cas.spline_Endoapd_BZ.interpolate(di)*factorCorrecAPD; 
      }
          
      // APD Memory
      new_apd = caseParams.apd_memory*apd + (1-caseParams.apd_memory)*new_apd;
      
      // Efecto electrotónico
      // Ajustar el nuevo apd en funcion del apd_medio de los vecinos
      float apd_medio = 0.0;
      if(Lv.size() == 0)
        apd_medio = new_apd;
      else {
        int vecMedia = 0;
        for (Node3 v: Lv){
          // Solo contamos vecinos activados que ya han actualizado su APD al estímulo actual
          if(v.estado == 1 || v.estado == 2){ // Para recursivo y eventos
              apd_medio+= v.apd;
              vecMedia++;
          }
        }
        
        if(vecMedia == 0)
          apd_medio = new_apd;
        else
          apd_medio/=vecMedia;
      }
      
      apd = new_apd*(1-kapd_v) + apd_medio*kapd_v;
      // Guardamos datos para obtener APD Máximo alcanzado en cada beat
      if(apd > apdMAX){
        apdMAX = apd;
        endo2EpiMAX = endo2Epi;
        tipoMAX = tipo;
        tiempo_transcurridoMAX = tiempo_transcurrido;
        idMAX = id;
      }
      
      // Calculamos velocidad de la conducción en sanas y BZ
      float new_cv = 0;
      // Si sanas
      if (tipo == 0){
        // Si Epi
        if (endo2Epi == 2 )
          new_cv = cas.spline_Epicv.interpolate(di);
        // Si M
        else if (endo2Epi == 1 )
          new_cv = cas.spline_Mcv.interpolate(di); 
        // Si Endo
        else 
          new_cv = cas.spline_Endocv.interpolate(di); 
      }
      // Si BZ
      else if (tipo == 1){
        // Si Epi
        if (endo2Epi == 2 )
          new_cv = (cas.spline_Epicv_BZ.interpolate(di))*factorCorrecCVBZ;
        // Si M
        else if (endo2Epi == 1 )
          new_cv = (cas.spline_Mcv_BZ.interpolate(di))*factorCorrecCVBZ; 
        // Si Endo
        else 
          new_cv = (cas.spline_Endocv_BZ.interpolate(di))*factorCorrecCVBZ;
      }  

      //if (new_cv < caseParams.vel_suelo)//Suelo de vel para no obtener datos irreales
        //new_cv = caseParams.vel_suelo;
      
      // CV memory
      cv = cv_memory*cv + (1-cv_memory)*new_cv;
    }
    // Si primera inicialización aumentamos APD para iniciliazación correcta y calculamos CV y APD sin memoria
    else{
      // Calcular el nuevo apd en funcion del di
      float new_apd = 0;
      if (tipo == 0){
        // Si Epi
        if (endo2Epi == 2 )
          new_apd = cas.spline_Epiapd.interpolate(di)*factorCorrecAPD;
        // Si M
        else if (endo2Epi == 1 )
          new_apd = cas.spline_Mapd.interpolate(di)*factorCorrecAPD; 
        // Si Endo
        else 
          new_apd = cas.spline_Endoapd.interpolate(di)*factorCorrecAPD; 
      }
      // Si BZ
      else if (tipo == 1){
        // Si Epi
        if (endo2Epi == 2 )
          new_apd = cas.spline_Epiapd_BZ.interpolate(di)*factorCorrecAPD;
        // Si M
        else if (endo2Epi == 1 )
          new_apd = cas.spline_Mapd_BZ.interpolate(di)*factorCorrecAPD; 
        // Si Endo
        else 
          new_apd = cas.spline_Endoapd_BZ.interpolate(di)*factorCorrecAPD; 
      }

      // Efecto electrotónico
      float apd_medio = 0.0;
      if(Lv.size() == 0)
        apd_medio = new_apd;
      else {
        int vecMedia = 0;
        for (Node3 v: Lv){
          // Solo contamos vecinos activados que ya han actualizado su APD al estímulo actual
          if(v.estado == 1 || v.estado == 2){ // Para recursivo y eventos
              apd_medio+= v.apd;
              vecMedia++;
          }
        }
        
        if(vecMedia == 0)
          apd_medio = new_apd;
        else
          apd_medio/=vecMedia;
      }
      
      apd = new_apd*(1-kapd_v) + apd_medio*kapd_v;
      
      // Calculamos la velocidad de la conducción en sanas y BZ
      if(tipo == 0){
        // Si Epi
        if (endo2Epi == 2 )
          cv = cas.spline_Epicv.interpolate(di);
        // Si M
        else if (endo2Epi == 1 )
          cv = cas.spline_Mcv.interpolate(di); 
        // Si Endo
        else 
          cv = cas.spline_Endocv.interpolate(di); 
      }
       // Si célula tipo BZ
      else if(tipo == 1) {
        // Si Epi
        if (endo2Epi == 2 )
          cv = cas.spline_Epicv_BZ.interpolate(di);
        // Si M
        else if (endo2Epi == 1 )
          cv = cas.spline_Mcv_BZ.interpolate(di); 
        // Si Endo
        else 
          cv = cas.spline_Endocv_BZ.interpolate(di);
      }
      //if (cv < caseParams.vel_suelo)//Suelo de vel para no obtener datos irreales
        //cv = caseParams.vel_suelo;
    }
  }
  
  void activar(){
    // Función de activar en el caso iterativo por eventos
    
    start_time = tiempo_transcurrido;
    periodo_activacion = start_time - last_start_time;
    
    calcularActivacion();

    estado = 2;
    t_proxima_activacion = INFINITO;
    t_proxima_desactivacion = tiempo_transcurrido + apd;
    t_proximo_evento = t_proxima_desactivacion;
    tvida = apd;
    if (lpath > ac.max_path){
      ac.max_path = lpath;
      ac.first = this;
    }
  }
  
  // Función de activar en el caso iterativo por eventos para bloque. Fija APD muy corto fijo y velocidad muy baja a 
  // todas las celdas del bloque por igual, aunque estén activadas
  void activar_BLK(EventQueue pq){
    // Desactivamos las celdas activadas antes de activarlas de nuevo para inicializar variables correctamente
    if (estado == 2){
      tvida = 0;
      last_start_time = start_time;
      start_time = INFINITO;
      end_time = tiempo_transcurrido;
      suma_tiempos_activadores = 0.0;
      tiempo_activador = 0.0;
      suma_activadores = new PVector(0,0,0);
      foco_activador = new PVector(0,0,0);
      normal_frente = new PVector(0,0,0);
      n_activadores = 0;
      potencial = 0;
      estado = -1;
      t_proxima_desactivacion = INFINITO;
      t_proximo_evento = INFINITO;
      lpath = 0;
      derror = 0;
      terror = 0;
      beat++;
    }
    // Activamos la celda
    start_time = tiempo_transcurrido;
    periodo_activacion = start_time - last_start_time;
    di = tiempo_transcurrido - end_time;
    apd = 125; // APD Fijo
    potencial += 1.0;
    
    //OPT 1 CV FIJO
    //cv = 0.0012; // CV Fijo???
    
    //OPT 2 CV Sano Endo Curva de Restitución
    cv = cas.spline_Endocv.interpolate(di);
    /*
    //OPT 3 CV con Memory Sano Endo Curva de Restitución
    float new_cv = cas.spline_Endocv.interpolate(di); 
    cv = cv_memory*cv + (1-cv_memory)*new_cv;
    */
    
    estado = 2;
    t_proxima_activacion = INFINITO;
    t_proxima_desactivacion = tiempo_transcurrido + apd;
    t_proximo_evento = t_proxima_desactivacion;
    tvida = apd;
    if (lpath > ac.max_path){
      ac.max_path = lpath;
      ac.first = this;
    }
    // Revisamos si activamos vecinos porque al entrar en disparaEvento ya estará activada y no entrará a comprobarlo
    // Contamos las celdas vecinas inactivas.
    // Vamos a repartir el potencial de acción entre ellas
    IntList inact = new IntList();
    for (int i=0;i<Lv.size();i++){
        Node3 vec = Lv.get(i);
        // Excluimos activar celulas vecinas del core para evitar cálculos
        if (vec.tipo != 2 ){
            //float tav = 0;
            PVector d = Lprv.get(i);
            float dist = Ldv.get(i);
            Evento ev = vec.propaga_activacion(this,this.lpath+dist,this.beat);
            
            if(ev != null) {
              pq.add(ev); // NEW_PQ: check
              inact.append(i);
            }
          }
      } 
      if(inact.size() > 0 ){
        float potnc = 1.0/inact.size();
        for(int i : inact) {
          Lv.get(i).potencial += potnc;
        }
      }
      estimulo_externo = false;
      pq.add(this.siguienteEvento()); // NEW_PQ: check
  }

  // incluir en activa bloque el recorrer los vecinos y pasarle a la función la lista de prioridad para añadir directamete los eventos 
  void dispara_evento(Evento e,EventQueue pq){
    
    if(tiempo_transcurrido == t_proxima_activacion )
    {
      if(estado != 0 || e.st != 0)
        println("La hemos liado al activar!  Estado "+estado+" debería ser 0. En el evento es "+e.st);

      if(!this.estimulo_externo && this.potencial < this.min_potencial*this.safety_factor)
      {
        if(this.potencial < this.min_potencial*this.safety_factor)
          println("Celda "+this.id+", estimulada por "+this.n_activadores+" no se activa. pot="+this.potencial+"  min_pot="+this.min_potencial);
        this.desactivar();
        return;
      }
      // Solo si el nodo a activar pertenece a la lista de init_nodes, inicializamos el valor al tiempo de beat actual, porque no tienen padres de activación
      // Y comprobamos que no lo está activando una reentrada, en tal caso el valor anterior de current_act_beat sería igual al beat actual LastStimTime (siempre que 
      // la reentrada ocurra después del último estímulo), en tal caso no se modifica el current_beat porque debe seguir siendo el último guardado
      if (ac.init_nodes.hasValue(id)){
        //println("ID INIT NODE: ", id);
        if (current_act_beat != ac.LastStimTime)
          current_act_beat = ac.LastStimTime; // Guardamos el tiempo del estímulo actual
        
        // Avisamos solo en el primer nodo init que lo detecte para que contador de ciclos de reentrada sea correcto. Para ello descartamos si contador ciclo 
        // de reentrada dentro de los siguientes 100 ms, ya que serán los nodos vecinos y pertenecerán al mismo ciclo ya contado
        // tCountNodesReen se inicializa a infinito y después se actualiza en cada ciclo a tiempo transcurrido 
        else if (tiempo_transcurrido - tCountNodesReen < 0 ||  tiempo_transcurrido - tCountNodesReen > 100){
          println("EN MISMO BEAT SE HAN VUELTO A ACTIVAR NODOS INICIALES SIN ESTÍMULO NUEVO - POSIBLE REENTRADA EN t:", tiempo_transcurrido);
          if (multiSim)
            outputSims.println("\nEN MISMO BEAT SE HAN VUELTO A ACTIVAR NODOS INICIALES SIN ESTÍMULO NUEVO - POSIBLE REENTRADA EN t " + tiempo_transcurrido +" Y Beat " + current_act_beat);
          tCountNodesReen = tiempo_transcurrido;
          countNodesReen++;
          
        }
      }
      activar();
      // Contamos las celdas vecinas inactivas.
      // Vamos a repartir el potencial de acción entre ellas
      IntList inact = new IntList();
      
      for (int i=0;i<Lv.size();i++){
        Node3 vec = Lv.get(i);
        // Excluimos activar celulas vecinas del core para evitar cálculos
        // y evitamos activar celdas con tiempos de activación altísimos
        if (vec.tipo != 2){
          //if (vec.tipo != 2  && di > 15){
            //float tav = 0;
            PVector d = Lprv.get(i);
            float dist = Ldv.get(i);
            /*
            //BZ Isotropico
            if(tipo == 1)
              tav = dist / cv;
            //Zona Sana Anisotropico
            else { 
              float vel_calc = calculaCV(d);
              tav = dist / vel_calc; // tiempo de activación del vecino = distancia al vecino / la velocidad de conducción de la propia celda obtenido a partir de su di
              //println("VEL CALC: ", vel_calc);
            }
            */
            Evento ev = vec.propaga_activacion(this,this.lpath+dist,this.beat);
            
            if(ev != null) {
              pq.add(ev);  // NEW_PQ: check
              inact.append(i);
            }
          }
        }
        
        if(inact.size() > 0 ){
          float potnc = 1.0/inact.size() * this.safety_factor;
          for(int i : inact) {
            Lv.get(i).potencial += potnc;
          }
        }
      estimulo_externo = false;
      
      pq.add(this.siguienteEvento()); // NEW_PQ: check

    }
    if(tiempo_transcurrido == t_proxima_desactivacion) {
      if(estado != 2 || e.st != 2 )
        println("La hemos liado al desactivar! Estado "+estado+" debería ser 2. En el evento es "+e.st);
      end_time = tiempo_transcurrido;
      Evento ev = desactivar();
      if( ev != null )
        pq.add(ev); // NEW_PQ: check
    }
      
  }

  
  // Método para crear lista de vecinos de cada nodo
  void add_vecino(Node3 v){
      PVector d = PVector.sub(v.pos, pos);
      Lprv.add(d);
      Ldv.append(d.mag());    // dst al vecino
      Lv.add(v);
      Lcv.append(1);
    
  }
  
  // Comprueba que no haya repetidos en l
  
  IntList getPath(){
    Node3 aux = this;
    IntList l = new IntList();
    while (aux != null){
      if (!l.hasValue(aux.id)) {
        l.append(aux.id);
        aux = aux.padre;
      }
      else{
          ac.re_nodes.appendUnique(aux.id);
          aux = null;
        }
    }
    return l;
  }
  
     
  
  void chusca_catheter(float radio_catheter, float dist ){
   
    for (int i=0;i<Lv.size();i++){
       Node3 hijo = Lv.get(i);
       if (dist + Ldv.get(i) < radio_catheter && !ac.init_nodes.hasValue(hijo.id)){
           ac.init_nodes.appendUnique(hijo.id);
           hijo.chusca_catheter(radio_catheter,dist+Ldv.get(i));        
       }
     }     
  }
 
}


    
  
