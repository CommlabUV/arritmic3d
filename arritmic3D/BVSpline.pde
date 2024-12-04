
    class BVSpline {
        private final float[] mX;
        private final float[] mY;
        private final float[][] mZ;
        private final float[][] mM;
         //public static int COUNT = 20;

        public BVSpline(float[] x, float[] y, float[][] z) {
            if (x == null || y == null || x.length != y.length || x.length < 2) {
                throw new IllegalArgumentException("Debe haber al menos dos puntos de control "
                        + "y los arrays deben tener la misma longitud.");
            }
            final int N = x.length;
            final int M = y.length;
            mM = new float[N-1][M-1];
            for (int i = 0; i < N-1; i++) {
              for (int j = 0; j < M-1; j++) {
                mM[i][j] = 1 / ((x[i+1] - x[i])*(y[j+1]-y[j]));
               //

            //  mM[i][j] = (y[j+1] - y[j]) / (x[i+1] - x[i]);
              }
            }
            mX = x;
            mY = y;
            mZ = z;
        }

         float[] d;
         int nearest[];

        public float interpolate(float x, float y) {


         // println("Value of DI " + x);
         // println("Value of APD0 " + y);
         // float apd = 0;

          final int n = mX.length;
          final int m = mY.length;
            if (Float.isNaN(x) || Float.isNaN(y)) {
              float aux = Float.NaN;
              return aux;
            }
           // apd = mZ[n][m];
           // println("Value of APD1 " + apd);
           // return apd;

            if (x <= mX[1] && y <= mY[1]) { //I think that mX[0] is the 0-value that stays in the intersection
                return mZ[1][1]; //se vai piu basso del primo valore, ne prendi uno costante
            }
            if (x >= mX[n-1] && y >= mY[m-1]) {
                return mZ[n-1][m-1]; //se vai piu alto del primo valore, ne prendi uno costante

              // return 260;
            }

            int x_prox = 1;
            int y_prox = 1;
            float dist_x = 1000;
            float dist_y = 1000;



           // int j = 0; j < rows; j++
            for (int i_x=1; i_x<n; i_x++){
              float dist = abs(x-mX[i_x]);
              if (dist<dist_x){
                dist_x = dist;
                x_prox = i_x;
              }
            }

           for (int i_y=1; i_y<m; i_y++){
              float dist = abs(y-mY[i_y]);
              if (dist<dist_y){
                dist_y = dist;
                y_prox = i_y;
              }
            }

          // println("Value of APD1 " + mZ[y_prox][x_prox]);
            return mZ[y_prox][x_prox];



          //  if (x >= mX[n-1] && y >= mY[m-1]) {

          //      println("ciao");
              // return 260;
          //  }
        //  println("Value of APD1 " + mZ);

            //float[][] p0;
            //float[][] p1;
            //float[][] pxy;
            //float[] d;

           /*
            int i = 0;
            int j = 0;
            while (x >= mX[i + 1] && y >= mY[j + 1]) {
                i += 1;
                j += 1;
                if (x == mX[i] && y == mY[j]) {
                   // mZ[i][j]=260;
                    return mZ[i][j];
                }else if (x == mX[i] && y != mY[j]){
                  mZ[i][j] = 260;
                 // for (int k = 1; k < m; k++) {
                  // d[k] = abs(y-mY[k]);
                 //   if (abs(y-mY[k]) < mY[m-1]/m && abs(y-mY[k-1])>abs(y-mY[k])){
                 //     j = k;
                 //   }
                //  }
                  return mZ[i][j];
                }else if (x != mX[i] && y == mY[j]){
                  mZ[i][j] = 260;
                //  for (int k = 1; k < n; k++) {
                  // d[k] = abs(x-mX[k]);
               //     if (abs(x-mX[k]) < mX[n-1]/n && abs(x-mX[k-1])>abs(x-mX[k])){
                 //     i = k;
                //    }
                //  }
                  return mZ[i][j];
                }else {
              //    p0 = mZ[i][j]+(mZ[i+1][j]-mZ[i][j])*((x-mX[i])/(mX[i+1]-mX[i]));
              //    p1 = mZ[i][j+1]+(mZ[i+1][j+1]-mZ[i][j+1])*((x-mX[i])/(mX[i+1]-mX[i]));
                 // mZ[i][j] = p0 + (p1-p0)*((y-mY[j])/(mY[j+1]-mY[j]));
                    mZ[i][j] = 260;
                    return mZ[i][j];
                }

              //  return mM[i][j] * (mZ[i][j]*(mX[i+1]-x)*(mY[j+1]-y)+mZ[i+1][j]*(x-mX[i])*(mY[j+1]-y)+mZ[i][j+1]*(mX[i+1]-x)*(y-mY[j])+mZ[i+1][j+1]*(x-mX[i])*(y-mY[j]));
            }
            //return mM[i][j] * (mZ[i][j]*(mX[i+1]-x)*(mY[j+1]-y)+mZ[i+1][j]*(x-mX[i])*(mY[j+1]-y)+mZ[i][j+1]*(mX[i+1]-x)*(y-mY[j])+mZ[i+1][j+1]*(x-mX[i])*(y-mY[j]));

          return mZ[i][j];
          */

        }


          public String toString() {
            StringBuilder str = new StringBuilder();
            final int n = mX.length;
            final int m = mY.length;
            str.append("BilinearSpline{[");
            for (int i = 0; i < n; i++) {
              for (int j = 0; j < m; j++) {
                if (i != 0 && j != 0) {
                    str.append(", ");
                }
                str.append("(").append(mX[i]);
                str.append("(").append(mY[i]);
                str.append(", ").append(mZ[i][j]);
                if (i < n-1 && j < m-1) {
                    str.append(": ").append(mM[i][j]);
                }
                str.append(")");
              }
            }
            str.append("]}");
            return str.toString();
        }
    }
