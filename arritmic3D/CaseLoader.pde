

class CaseLoader {

  String path, m3dPath;
  ArrayList<String> fichs;
  // Información del caso: Centroide de cada nodo, nivel de gris RMI, lista de vecinos,
  // curva de restitución APD, curva de restitución CV, superficie de restitución APD, superficie de restitución CV, orientación de fibras y tipo de célula (sana, BZ, escara)
  FloatList X,Y,Z,P, oX, oY, oZ, vertex_ratio, vertex_reductCV;
  IntList cellType;
  IntList endo2Epi;
  PVector centerMass;
  int bpl_CasoBloqueX; // Bloques por lado en X que viene con el caso de bloque vtk para no tener que modificarlo
  int bpl_CasoBloqueY; // Bloques por lado en Y que viene con el caso de bloque vtk para no tener que modificarlo
  int capasZCasoBloque; // Capas en Z de caso bloque VTK
  IntList nodes_CasoBloqueVent; // nodos del Bloque que viene con el caso de ventriculo/auricula
  float cellsize_bloqueVTK; // Tamaño de celda en mm para Bloque VTK
  ArrayList<IntList> V;
  int nCeldas;
  int rows, cols;
  Spline spline_Mapd, spline_Mcv, spline_Epiapd, spline_Epicv,spline_Endoapd,spline_Endocv,spline_Mapd_BZ, spline_Mcv_BZ, spline_Epiapd_BZ, spline_Epicv_BZ,spline_Endoapd_BZ,spline_Endocv_BZ;
  BVSpline bvspline_Mapd, bvspline_Mcv, bvspline_Epiapd, bvspline_Epicv, bvspline_Endoapd, bvspline_Endocv, bvspline_Mapd_BZ, bvspline_Mcv_BZ, bvspline_Epiapd_BZ, bvspline_Epicv_BZ, bvspline_Endoapd_BZ, bvspline_Endocv_BZ;
    //G: the next line should be the same, we'll just have to add APD0 near to DI
  ArrayList<PVector> M_APD, M_CV, Epi_APD,Epi_CV, Endo_APD,Endo_CV, BZ_M_APD,BZ_M_CV, BZ_Epi_APD,BZ_Epi_CV, BZ_Endo_APD,BZ_Endo_CV;   // APD, CV curves

  float[][] MM_APD, EEpi_APD,EEndo_APD,BZ_MM_APD,BZ_EEpi_APD,BZ_EEndo_APD;

  CaseLoader(String casePath, String m3DFile) {

    this.path = casePath;
    this.m3dPath = m3DFile;

    ///All the file are saved as fichs
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

    ////G:
    fichs.add(skecthPath+"restitutionSurfaces/RestitutionSurface_Sanas_APD_Mid.csv"); // fichs 24  ** M **
    //fichs.add(skecthPath+"restitutionSurfaces/RestitutionSurface_Sanas_CV_Mid.csv"); // fichs 25  ** M **

    fichs.add(skecthPath+"restitutionSurfaces/RestitutionSurface_Sanas_APD_Epi.csv"); // fichs 25
    //fichs.add(skecthPath+"restitutionSurfaces/RestitutionSurface_Sanas_CV_Epi.csv"); // fichs 27

    fichs.add(skecthPath+"restitutionSurfaces/RestitutionSurface_Sanas_APD_Endo.csv"); // fichs 28
    //fichs.add(skecthPath+"restitutionSurfaces/RestitutionSurface_Sanas_CV_Endo.csv"); // fichs 29

    fichs.add(skecthPath+"restitutionSurfaces/RestitutionSurface_BZ_APD_Endo.csv"); // fichs 27
    //fichs.add(skecthPath+"restitutionSurfaces/RestitutionSurface_BZ_CV_Endo.csv"); // fichs 31

    fichs.add(skecthPath+"restitutionSurfaces/RestitutionSurface_BZ_APD_Epi.csv"); // fichs 28
    //fichs.add(skecthPath+"restitutionSurfaces/RestitutionSurface_BZ_CV_Epi.csv"); // fichs 33

    fichs.add(skecthPath+"restitutionSurfaces/RestitutionSurface_BZ_APD_Mid.csv"); // fichs 29
    //fichs.add(skecthPath+"restitutionSurfaces/RestitutionSurface_BZ_CV_Mid.csv"); // fichs 35


    fichs.add(casePath+"Reader_VTK/vertex_DlongDtransratio.txt"); // fichs 30 //Ratio Dlong/Dtrans = CVlon/CVtrans for each vertex
    fichs.add(casePath+"Reader_VTK/vertex_reductCV.txt"); // fichs 31 //ReductCV for each vertex
    fichs.add(casePath+"Reader_VTK/nodes_CasoBloqueVent.txt"); // fichs 32 //Nodes of the block of the Atrium to generate reentrance

    load_case_data();

  }

//  void setup() {
//  noLoop();
//  centerMass = new PVector(float, float);
//  centerMass1 = new PVector(float, float, float, float, float, float);
// }


  ArrayList<PVector> carga_csv(String fich_csv){
    Table t;
    ArrayList<PVector> l = new ArrayList<PVector>();
      t = loadTable(fich_csv);
      if(t != null){
        for (TableRow row : t.rows())
         //  if (t.getColumnCount()<=2) {
            l.add( new PVector(row.getFloat(0), row.getFloat(1)) );
         //  } else {
         //   l.add(new PVector(row.getFloat(0), row.getFloat(1), row.getFloat(2)));
         //  }
        println(t.getRowCount() + " values for "+fich_csv);
      }
      return l;
   }

   float [][] carga_csv_surf(String fich_csv){
       Table t;
       t = loadTable(fich_csv);
       rows = t.getRowCount();
       cols = t.getColumnCount();
       float[][] matrix = new float[rows][cols];
          //t = loadTable(fich_csv);
          if(t != null){
           for (int i = 0; i < cols; i++) {
              for (int j = 0; j < rows; j++) {
                matrix[i][j] = t.getFloat(i, j);
                if (i==1 && j==1)
                    println(t.getFloat(i,j)+ "first value APD+1 of " + fich_csv);
                if (i==rows-1 && j==cols-1)
                    println(t.getFloat(i,j)+ "last APD+1 value of " + fich_csv);
                //println(t.getRowCount() + " values for "+fich_csv);
              }
              }
     }
     return matrix;
   }

  // size(6,6);
 // int cols = 6;
 // int rows = 6;

  // Declare 2D array
 // int[][] myArray = new int[cols][rows];

  // Initialize 2D array values
//for (int i = 0; i < cols; i++) {
//  for (int j = 0; j < rows; j++) {
//    myArray[i][j] = 0;
//  }
//}

//  ArrayList<IntList> carga_csv_surf(String fich_csv){
//    Table t;
//    ArrayList<IntList> m = new ArrayList<IntList>();
//      t = loadTable(fich_csv);
//      if(t != null){
//        for (TableRow row : t.rows())
//            m.add(new IntList(row.getFloat(0), row.getFloat(1), row.getFloat(2), row.getFloat(3), row.getFloat(4), row.getFloat(5)) );
//        println(t.getRowCount() + " values for "+fich_csv);
//      }
//      return m;
//   }


   //  ArrayList<PVector1> carga_csv1(String fich_csv){
   // Table t;
   // ArrayList<PVector1> l = new ArrayList<PVector1>();
    //  t = loadTable(fich_csv);
    //  if(t != null){
    //    for (TableRow row : t.rows())
    //    l.add( new PVector(row.getFloat(0), row.getFloat(1), row.getFloat(2), row.getFloat(3), row.getFloat(4), row.getFloat(5)) );
    //
   //     println(t.getRowCount() + " values for "+fich_csv);
   //   }
  //    return l;
  // }

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

   BVSpline genera_bvspline(float[][] data_surf){
      rows = data_surf.length;
      cols = data_surf[0].length;
      float Xsp[] = new float[cols];
      float Ysp[] = new float[rows];
      float Zsp[][] = new float[rows][cols];
       for (int i = 1; i < cols; i++) {
          for (int j = 1; j < rows; j++) {
            Xsp[i] = data_surf[0][i];
            Ysp[j] = data_surf[j][0];
            //i+=1;
            //j+=1;
           // println(Xsp[i]);
           // println(Ysp[j]);
          }
       }

        for (int i = 0; i < cols; i++) {
          for (int j = 0; j < rows; j++) {
            Zsp[i][j] = data_surf[i][j];
            //j+=1;
            //println(Zsp[i][j]);
          }
         // i+=1;
          //println(Zsp[i][j]);
       }
      //println(Zps);
      return new BVSpline(Xsp, Ysp, Zsp);

   }

  // BVSpline genera_bvspline(ArrayList<PVector> data){
  //    PVector[][] data_name = new PVector[row][col];
  //    row = data_name.length;
  //    col = data_name[0].length;
  //    float Xsp[] = new float[data_name[0].length];
  //    float Ysp[] = new float[data_name.length];
  //    float Zsp[] = new float[data.size()];
  //    int i = 0;
  //    for (PVector p: data){
  //      Xsp[i] = p.x;
  //      Ysp[i] = p.y;
  //      Zsp[i] = p.z;
  //      i+=1;
  //    }

  //    return new BVSpline(Xsp, Ysp, Zsp);

  // }

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
    vertex_ratio = new FloatList();
    vertex_reductCV = new FloatList();
    cellType = new IntList();
    endo2Epi = new IntList();
    nodes_CasoBloqueVent = new IntList();
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

    MM_APD  = carga_csv_surf(fichs.get(24));
    if(MM_APD.length != 0)
      bvspline_Mapd = genera_bvspline(MM_APD);
    else
      println("\nError al leer "+fichs.get(24)+"! No se ha podido cargar la Superficie de APD para M. Al menos esta superficie debe estar presente.");
  //  MM_CV = carga_csv(fichs.get(25));
  //  if(MM_CV.size() != 0)
  //    bvspline_Mcv = genera_bvspline(MM_CV);
  //  else
  //   println("\nError al leer "+fichs.get(25)+"! No se ha podido cargar la Superficie de CV para M. Al menos esta superficie debe estar presente.");

    // Epi
    Epi_APD  = carga_csv(fichs.get(11));
    if(Epi_APD.size() != 0)
      spline_Epiapd = genera_spline(Epi_APD);
    else
      println("\nError al leer "+fichs.get(11)+"! No se ha podido cargar la curva de APD para Epi. Al menos esta curva debe estar presente.");
    Epi_CV = carga_csv(fichs.get(12));
    if(Epi_CV.size() != 0)
      spline_Epicv = genera_spline(Epi_CV);
    else
     println("\nError al leer "+fichs.get(12)+"! No se ha podido cargar la curva de CV para Epi. Al menos esta curva debe estar presente.");

    EEpi_APD  = carga_csv_surf(fichs.get(25));
    if(EEpi_APD.length != 0)
      bvspline_Epiapd = genera_bvspline(EEpi_APD);
    else
      println("\nError al leer "+fichs.get(25)+"! No se ha podido cargar la superficie de APD para Epi. Al menos esta superficie debe estar presente.");
    //EEpi_CV = carga_csv(fichs.get(27));
    //if(EEpi_CV.size() != 0)
    //  bvspline_Epicv = genera_bvspline(Epi_CV);
    //else
    //  println("\nError al leer "+fichs.get(27)+"! No se ha podido cargar la superficie de CV para Epi. Al menos esta superficie debe estar presente.");

    // Endo
    Endo_APD  = carga_csv(fichs.get(13));
    if(Endo_APD.size() != 0)
      spline_Endoapd = genera_spline(Endo_APD);
    else
      println("\nError al leer "+fichs.get(13)+"! No se ha podido cargar la curva de APD para Endo. Al menos esta curva debe estar presente.");
    Endo_CV = carga_csv(fichs.get(14));
    if(Endo_CV.size() != 0)
      spline_Endocv = genera_spline(Endo_CV);
    else
      println("\nError al leer "+fichs.get(14)+"! No se ha podido cargar la curva de CV para Endo. Al menos esta curva debe estar presente.");

    EEndo_APD  = carga_csv_surf(fichs.get(26));
    if(EEndo_APD.length != 0)
      bvspline_Endoapd = genera_bvspline(EEndo_APD);
      //println("\nEstoy leiendo "+fichs.get(26)+"!");
    else
      println("\nError al leer "+fichs.get(26)+"! No se ha podido cargar la superficie de APD para Endo. Al menos esta superficie debe estar presente.");
   // EEndo_CV = carga_csv(fichs.get(29));
   // if(EEndo_CV.size() != 0)
   //   bvspline_Endocv = genera_bvspline(Endo_CV);
   // else
   //   println("\nError al leer "+fichs.get(29)+"! No se ha podido cargar la superficie de CV para Endo. Al menos esta superficie debe estar presente.");

    // Curvas de restitucion BZ
    // Endo
    BZ_Endo_APD  = carga_csv(fichs.get(16));
    if(BZ_Endo_APD.size() != 0)
      spline_Endoapd_BZ = genera_spline(BZ_Endo_APD);
    else
      println("\nError al leer "+fichs.get(16)+"! No se ha podido cargar la curva de APD para Endo_BZ. Al menos esta curva debe estar presente.");
    BZ_Endo_CV = carga_csv(fichs.get(17));
    if(BZ_Endo_CV.size() != 0)
      spline_Endocv_BZ = genera_spline(BZ_Endo_CV);
    else
      println("\nError al leer "+fichs.get(17)+"! No se ha podido cargar la curva de CV para Endo_BZ. Al menos esta curva debe estar presente.");

    BZ_EEndo_APD  = carga_csv_surf(fichs.get(27));
    if(BZ_EEndo_APD.length != 0)
      bvspline_Endoapd_BZ = genera_bvspline(BZ_EEndo_APD);
    else
      println("\nError al leer "+fichs.get(27)+"! No se ha podido cargar la superficie de APD para Endo_BZ. Al menos esta superficie debe estar presente.");
  //  BZ_EEndo_CV = carga_csv(fichs.get(31));
  // if(BZ_EEndo_CV.size() != 0)
  //    bvspline_Endocv_BZ = genera_bvspline(BZ_Endo_CV);
  //  else
  //    println("\nError al leer "+fichs.get(31)+"! No se ha podido cargar la superficie de CV para Endo_BZ. Al menos esta superficie debe estar presente.");

    // Epi
    BZ_Epi_APD  = carga_csv(fichs.get(18));
    if(BZ_Epi_APD.size() != 0)
      spline_Epiapd_BZ = genera_spline(BZ_Epi_APD);
    else
      println("\nError al leer "+fichs.get(18)+"! No se ha podido cargar la curva de APD para Epi_BZ. Al menos esta curva debe estar presente.");
    BZ_Epi_CV = carga_csv(fichs.get(19));
    if(BZ_Epi_CV.size() != 0)
      spline_Epicv_BZ = genera_spline(BZ_Epi_CV);
    else
      println("\nError al leer "+fichs.get(19)+"! No se ha podido cargar la curva de CV para Epi_BZ. Al menos esta curva debe estar presente.");

    BZ_EEpi_APD  = carga_csv_surf(fichs.get(28));
    if(BZ_EEpi_APD.length != 0)
      bvspline_Epiapd_BZ = genera_bvspline(BZ_EEpi_APD);
    else
      println("\nError al leer "+fichs.get(28)+"! No se ha podido cargar la superficie de APD para Epi_BZ. Al menos esta superficie debe estar presente.");
   // BZ_EEpi_CV = carga_csv(fichs.get(33));
   // if(BZ_EEpi_CV.size() != 0)
   //   bvspline_Epicv_BZ = genera_bvspline(BZ_Epi_CV);
   // else
   //   println("\nError al leer "+fichs.get(33)+"! No se ha podido cargar la superficie de CV para Epi_BZ. Al menos esta superficie debe estar presente.");

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

    BZ_MM_APD  = carga_csv_surf(fichs.get(29));
    if(BZ_MM_APD.length != 0)
      bvspline_Mapd_BZ = genera_bvspline(BZ_MM_APD);
    else
      println("\nError al leer "+fichs.get(29)+"! No se ha podido cargar la superficie de APD BZ para M. Al menos esta superficie debe estar presente.");
   // BZ_MM_CV = carga_csv(fichs.get(35));
   // if(BZ_MM_CV.size() != 0)
   //   bvspline_Mcv_BZ = genera_bvspline(BZ_M_CV);
   // else
   //   println("\nError al leer "+fichs.get(35)+"! No se ha podido cargar la superficie de CV BZ para M. Al menos esta superficie debe estar presente.");


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


        // Dlong/Dtrans
    lines = loadStrings(fichs.get(30));
    if(lines != null){
      for (int i = 0 ; i < lines.length; i++){
        float ciao = float(lines[i]);
        vertex_ratio.append(ciao);
      }
      println("Leido: ", fichs.get(30));
      nCeldas = lines.length;
      println("Nceldas: ", nCeldas);
    }else{
      println("File "+ fichs.get(30)+ " could not be read.");
    }

    // nodes bloque ventriculo/auricula
    lines = loadStrings(fichs.get(32));
    if(lines != null){
      for (int i = 0 ; i < lines.length; i++){
        nodes_CasoBloqueVent.append(round(float(lines[i])));
      }
      println("Leido: ", fichs.get(32));
      nCeldas = lines.length;
      println("NceldasBloque: ", nCeldas);
    }else{
      println("File "+ fichs.get(32)+ " could not be read.");
    }

            // reductCV
    lines = loadStrings(fichs.get(31));
    if(lines != null){
      for (int i = 0 ; i < lines.length; i++){
        float ciaoo = float(lines[i]);
        vertex_reductCV.append(ciaoo);
      }
      println("Leido: ", fichs.get(31));
      nCeldas = lines.length;
      println("Nceldas: ", nCeldas);
    }else{
      println("File "+ fichs.get(31)+ " could not be read.");
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


