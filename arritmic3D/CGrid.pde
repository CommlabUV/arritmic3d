class GridCell{
  PVector center;
  ArrayList<Node3> Lnodes_in;

  GridCell(PVector pos){
    center = pos;
    Lnodes_in =  new ArrayList<Node3>();
  }

}

class CGrid{
  float      tam_cell, tam_grid;
  int        cpl; // celdas por lado
  PVector    center;
  GridCell   M[][][];

  CGrid(float tgrid, float tcelda){

    center   = new PVector();
    tam_cell = tcelda;
    tam_grid = tgrid;

    cpl = int(tam_grid/tam_cell)+1;
    M = new GridCell[cpl][cpl][cpl];

    for(int x =0; x < cpl; x++)
      for(int y =0; y < cpl; y++)
         for(int z =0; z< cpl; z++){

           PVector pos = new PVector(x*tam_cell, y*tam_cell, z*tam_cell);

            GridCell new_cell = new GridCell(pos);
            M[x][y][z] = new_cell;

         }

  }

  void reset(){

    for(int x =0; x < cpl; x++)
      for(int y =0; y < cpl; y++)
         for(int z =0; z< cpl; z++)
           M[x][y][z].Lnodes_in.clear();
  }

  void setCenter(PVector c){
    center = c;
  }
  void addNode(Node3 n){

    // Método para calcular la celda a la que pertenece
    int x = int(cpl/2 + int((n.pos.x - center.x)) / tam_cell);
    int y = int(cpl/2 + int((n.pos.y - center.y)) / tam_cell);
    int z = int(cpl/2 + int((n.pos.z - center.z)) / tam_cell);

    if ((x < 0  || x > cpl) || (y < 0  || y > cpl) || (z < 0  || z > cpl)){
      println("NOINS:: grid:add at "+str(x)+", "+str(y)+", "+str(z)+ " : nodo ", n.id);
      println(cpl, tam_cell);
      exit();
      return;
    }
    else{
      M[x][y][z].Lnodes_in.add(n);
      //println("grid:add at "+str(x)+", "+str(y)+", "+str(z)+ " : nodo ", n.id);
    }
  }


  //mode: 0 todas las celdas; 1 celdas activas
  void draw(int mode){

    strokeWeight(caseParams.voxel_size*0.1);
    stroke(20);
    noFill();
    pushMatrix();

    translate(center.x-tam_grid/2, center.y-tam_grid/2, center.z-tam_grid/2);

    for(int i=0; i<cpl; i++){
      for(int j=0; j<cpl; j++){
        for(int k=0; k<cpl; k++){
          if (mode == 0 || (mode == 1 && M[i][j][k].Lnodes_in.size() > 0) ){
            pushMatrix();
            translate(i*tam_cell,j*tam_cell, k*tam_cell);
            box(tam_cell);
            popMatrix();
          }
        }
      }
    }
    popMatrix();

    strokeWeight(caseParams.voxel_size);

  }

}
