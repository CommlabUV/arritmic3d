
    
class CaseLoader {
  
  String path, m3dPath;
  ArrayList<String> fichs;
  // Información del caso: Centroide de cada nodo, nivel de gris RMI, lista de vecinos, 
  // curva de restitución APD, curva de restitución CV, orientación de fibras y tipo de célula (sana, BZ, escara)
  FloatList X,Y,Z,P, oX, oY, oZ;
  IntList cellType;
  IntList endo2Epi;
  PVector centerMass;
  int bpl_CasoBloqueX; // Bloques por lado en X que viene con el caso de bloque vtk para no tener que modificarlo
  int bpl_CasoBloqueY; // Bloques por lado en Y que viene con el caso de bloque vtk para no tener que modificarlo
  int capasZCasoBloque; // Capas en Z de caso bloque VTK
  float cellsize_bloqueVTK; // Tamaño de celda en mm para Bloque VTK
  ArrayList<IntList> V; 
  int nCeldas;
  Spline spline_Mapd, spline_Mcv, spline_Epiapd, spline_Epicv,spline_Endoapd,spline_Endocv,spline_Mapd_BZ, spline_Mcv_BZ, spline_Epiapd_BZ, spline_Epicv_BZ,spline_Endoapd_BZ,spline_Endocv_BZ;
 
  ArrayList<PVector> M_APD,M_CV, Epi_APD,Epi_CV,Endo_APD,Endo_CV,BZ_M_APD,BZ_M_CV, BZ_Epi_APD,BZ_Epi_CV,BZ_Endo_APD,BZ_Endo_CV;       // APD, CV curves
  CaseLoader(String casePath, String m3DFile) {
  
    this.path = casePath;
    this.m3dPath = m3DFile;
    
    fichs = new ArrayList<String>();
    fichs.add(casePath+"Reader_VTK/centroidCellX.txt"); // fichs 0
    fichs.add(casePath+"Reader_VTK/centroidCellY.txt"); // fichs 1
    fichs.add(casePath+"Reader_VTK/centroidCellZ.txt"); // fichs 2
    
    fichs.add(casePath+"Reader_VTK/scalarValues.txt"); // fichs 3
    fichs.add(casePath+"Reader_VTK/vecinos.txt"); // fichs 4
    
    fichs.add(skecthPath+"restitutionCurves/RestitutionCurve_Sanas_APD_Mid.csv"); // fichs 5  ** M **
    fichs.add(skecthPath+"restitutionCurves/RestitutionCurve_Sanas_CV_Mid.csv"); // fichs 6  ** M **

    fichs.add(casePath+"Reader_VTK/fibreOrientationX.txt");  // fichs 7
    fichs.add(casePath+"Reader_VTK/fibreOrientationY.txt"); // fichs 8
    fichs.add(casePath+"Reader_VTK/fibreOrientationZ.txt"); // fichs 9
    
    fichs.add(casePath+"Reader_VTK/scarTissue.txt"); // fichs 10
    
    fichs.add(skecthPath+"restitutionCurves/RestitutionCurve_Sanas_APD_Epi.csv"); // fichs 11
    fichs.add(skecthPath+"restitutionCurves/RestitutionCurve_Sanas_CV_Epi.csv"); // fichs 12
    
    fichs.add(skecthPath+"restitutionCurves/RestitutionCurve_Sanas_APD_Endo.csv"); // fichs 13
    fichs.add(skecthPath+"restitutionCurves/RestitutionCurve_Sanas_CV_Endo.csv"); // fichs 14
    
    fichs.add(casePath+"Reader_VTK/EndoToEpi.txt"); // fichs 15
    
    fichs.add(skecthPath+"restitutionCurves/RestitutionCurve_BZ_APD_Endo.csv"); // fichs 16
    fichs.add(skecthPath+"restitutionCurves/RestitutionCurve_BZ_CV_Endo.csv"); // fichs 17
    
    fichs.add(skecthPath+"restitutionCurves/RestitutionCurve_BZ_APD_Epi.csv"); // fichs 18
    fichs.add(skecthPath+"restitutionCurves/RestitutionCurve_BZ_CV_Epi.csv"); // fichs 19
    
    fichs.add(skecthPath+"restitutionCurves/RestitutionCurve_BZ_APD_Mid.csv"); // fichs 20
    fichs.add(skecthPath+"restitutionCurves/RestitutionCurve_BZ_CV_Mid.csv"); // fichs 21
    
    fichs.add(casePath+"Reader_VTK/centerMass-BPL.txt"); // fichs 22
    
    fichs.add(casePath+"Reader_VTK/pacing_34.txt"); // fichs 23
  
    load_case_data();
  }
  
  
    
  ArrayList<PVector> carga_csv(String fich_csv){
    Table t;
    ArrayList<PVector> l = new ArrayList<PVector>();
      t = loadTable(fich_csv);
      if(t != null){
        for (TableRow row : t.rows()) 
        l.add( new PVector(row.getFloat(0), row.getFloat(1)) );
       
        println(t.getRowCount() + " values for "+fich_csv);
      }
      return l;
   }
   
   Spline genera_spline(ArrayList<PVector> data){
      float Xsp[] = new float[data.size()];
      float Ysp[] = new float[data.size()];
      int i = 0;
      for (PVector p: data){
        Xsp[i] = p.x;
        Ysp[i] = p.y;
        i+=1;
      }
      
      return new Spline(Xsp, Ysp);
      
   }
   
  void load_case_data(){
    
    // Posicion, vecinos, Nivel de gris, orientacion de fibras, tipo de célula (sana, BZ, escara)
    X = new FloatList();
    Y = new FloatList();
    Z = new FloatList();
    V = new ArrayList<IntList>();
    P = new FloatList();
    oX = new FloatList();
    oY = new FloatList();
    oZ = new FloatList();
    cellType = new IntList();
    endo2Epi = new IntList();
    centerMass = new PVector();
    
   
      
    // Posicion
    String[] lines = loadStrings(fichs.get(0));
    for (int i = 0 ; i < lines.length; i++)
      X.append(float(lines[i]));
    println("Leido: ", fichs.get(0));
    nCeldas = lines.length;
    println("Nceldas: ", nCeldas);
    
    lines = loadStrings(fichs.get(1));
    for (int i = 0 ; i < lines.length; i++)
      Y.append(float(lines[i]));
    println("Leido: ", fichs.get(1));
    nCeldas = lines.length;
    println("Nceldas: ", nCeldas);
    
    lines = loadStrings(fichs.get(2));
    for (int i = 0 ; i < lines.length; i++)
      Z.append(float(lines[i]));
    println("Leido: ", fichs.get(2));
    nCeldas = lines.length;
    println("Nceldas: ", nCeldas);
    
    // Vecinos
    lines = loadStrings(fichs.get(4));
    for (int i = 0 ; i < lines.length; i++){
       String[] tok = split(lines[i], ' ');
       IntList lv = new IntList();
       for (String v: tok)
         if (v.length() > 0)
           lv.append(int(v));
       V.add(lv);
       }
    println("Leido: ", fichs.get(4));
    nCeldas = lines.length;
    println("Nceldas: ", nCeldas);
    
    // Grayscale
    lines = loadStrings(fichs.get(3));
    if(lines != null){
      for (int i = 0 ; i < lines.length; i++){
        float scalar = float(lines[i]);
        P.append(scalar);
      }
      println("Leido: ", fichs.get(3));
      nCeldas = lines.length;
      println("Nceldas: ", nCeldas);
    }else{
      println("No grayscale file found.");
    }

    // Curvas de restitutcion Células Sanas
    // M
    M_APD  = carga_csv(fichs.get(5));
    if(M_APD.size() != 0)
      spline_Mapd = genera_spline(M_APD);
    else
      println("\nError al leer "+fichs.get(5)+"! No se ha podido cargar la curva de APD para M. Al menos esta curva debe estar presente.");
    M_CV = carga_csv(fichs.get(6));
    if(M_CV.size() != 0)
      spline_Mcv = genera_spline(M_CV);
    else
      println("\nError al leer "+fichs.get(5)+"! No se ha podido cargar la curva de CV para M. Al menos esta curva debe estar presente.");
    // Epi
    Epi_APD  = carga_csv(fichs.get(11));
    if(Epi_APD.size() != 0)
      spline_Epiapd = genera_spline(Epi_APD);
    Epi_CV = carga_csv(fichs.get(12));
    if(Epi_CV.size() != 0)
      spline_Epicv = genera_spline(Epi_CV);
    // Endo
    Endo_APD  = carga_csv(fichs.get(13));
    if(Endo_APD.size() != 0)
      spline_Endoapd = genera_spline(Endo_APD);
    Endo_CV = carga_csv(fichs.get(14));
    if(Endo_CV.size() != 0)
      spline_Endocv = genera_spline(Endo_CV);
   
    // Curvas de restitucion BZ
    // Endo
    BZ_Endo_APD  = carga_csv(fichs.get(16));
    if(BZ_Endo_APD.size() != 0)
      spline_Endoapd_BZ = genera_spline(BZ_Endo_APD);
    BZ_Endo_CV = carga_csv(fichs.get(17));
    if(BZ_Endo_CV.size() != 0)
      spline_Endocv_BZ = genera_spline(BZ_Endo_CV);
    // Epi
    BZ_Epi_APD  = carga_csv(fichs.get(18));
    if(BZ_Epi_APD.size() != 0)
      spline_Epiapd_BZ = genera_spline(BZ_Epi_APD);
    BZ_Epi_CV = carga_csv(fichs.get(19));
    if(BZ_Epi_CV.size() != 0)
      spline_Epicv_BZ = genera_spline(BZ_Epi_CV);
    // M
    BZ_M_APD  = carga_csv(fichs.get(20));
    if(BZ_M_APD.size() != 0)
      spline_Mapd_BZ = genera_spline(BZ_M_APD);
    else
      println("\nError al leer "+fichs.get(20)+"! No se ha podido cargar la curva de APD BZ  para M. Al menos esta curva debe estar presente.");
    BZ_M_CV = carga_csv(fichs.get(21));
    if(BZ_M_CV.size() != 0)
      spline_Mcv_BZ = genera_spline(BZ_M_CV);
    else
      println("\nError al leer "+fichs.get(21)+"! No se ha podido cargar la curva de CV BZ para M. Al menos esta curva debe estar presente.");
    
    
    // Orientacion de fibras y tipo de célula Sana, BZ, Scar
    lines = loadStrings(fichs.get(7));
    if(lines != null){
      for (int i = 0 ; i < lines.length; i++)
        oX.append(float(lines[i]));
      println("Leido: ", fichs.get(7));
      nCeldas = lines.length;
      println("Nceldas: ", nCeldas);
    }else{
      println("File "+ fichs.get(7)+ " could not be read.");
    }
      
    lines = loadStrings(fichs.get(8));
    if(lines != null){
      for (int i = 0 ; i < lines.length; i++)
        oY.append(float(lines[i]));
      println("Leido: ", fichs.get(8));
      nCeldas = lines.length;
      println("Nceldas: ", nCeldas);
    }else{
      println("File "+ fichs.get(8)+ " could not be read.");
    }
    
    lines = loadStrings(fichs.get(9));
    if(lines != null){
      for (int i = 0 ; i < lines.length; i++)
        oZ.append(float(lines[i]));
      println("Leido: ", fichs.get(9));
      nCeldas = lines.length;
      println("Nceldas: ", nCeldas);
    }else{
      println("File "+ fichs.get(9)+ " could not be read.");
    }

    // Tipo de celula, scar 2, BZ 1, sana 0 
    lines = loadStrings(fichs.get(10));
    if(lines != null){
      for (int i = 0 ; i < lines.length; i++)
        cellType.append(round(float(lines[i])));
      println("Leido: ", fichs.get(10));
      nCeldas = lines.length;
      println("Nceldas: ", nCeldas);
    }else{
      println("File "+ fichs.get(10)+ " could not be read.");
    }
    
    // Tipo de celula Endo To Epi, epi 2, M 1, endo 0 
    lines = loadStrings(fichs.get(15));
    if(lines != null){
      for (int i = 0 ; i < lines.length; i++)
        endo2Epi.append(round(float(lines[i])));
      println("Leido: ", fichs.get(15));
      nCeldas = lines.length;
      println("Nceldas: ", nCeldas);
    }else{
      println("File "+ fichs.get(15)+ " could not be read.");
    }
    // Centro de masas para ajustar la cámara
    lines = loadStrings(fichs.get(22));
    centerMass.x = float(lines[0]);
    centerMass.y = float(lines[1]);
    centerMass.z = float(lines[2]);
    // Si es un caso de Bloque vtk llevará 4 líneas más con num de celdas por lado X-Y-Z y tamaño de celda en mm
    if (lines.length > 3){
      bpl_CasoBloqueX = int(lines[3]);
      bpl_CasoBloqueY = int(lines[4]);
      capasZCasoBloque = int(lines[5]);
      cellsize_bloqueVTK = float(lines[6]);
    }
    println("Leido: ", fichs.get(22));
    
    // Si no se detalla en params los nodos de pacing, se asignan los 34 nodos Endo Epi de los 17 segmentos AHA 
    if (Float.isNaN(caseParams.id_extraIMulti.get(0))){
      caseParams.id_extraIMulti.clear();
      lines = loadStrings(fichs.get(23));
      if(lines != null){
        for (int i = 0 ; i < lines.length; i++)
          if (float(lines[i]) != -1.)
            caseParams.id_extraIMulti.append(float(lines[i]));
        println("Leido: ", fichs.get(23));
      }else{
        println("File "+ fichs.get(23)+ " could not be read.");
      }
    }
  }
}
  
  
