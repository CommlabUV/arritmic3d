/**
 * Realiza interpolación de splines dado un conjunto de puntos de control.
 */
class Spline {
        private final float[] mX;
        private final float[] mY;
        private final float[] mM;
        public Spline(float[] x, float[] y) {
            if (x == null || y == null || x.length != y.length || x.length < 2) {
                throw new IllegalArgumentException("Debe haber al menos dos puntos de control "
                        + "y los arrays deben tener la misma longitud.");
            }
            final int N = x.length;
            mM = new float[N-1];
            for (int i = 0; i < N-1; i++) {
                mM[i] = (y[i+1] - y[i]) / (x[i+1] - x[i]);
            }
            mX = x;
            mY = y;
        }
       
        public float interpolate(float x) {
            final int n = mX.length;
            if (Float.isNaN(x)) {
                return x;
            }
            if (x <= mX[0]) {
                return mY[0];
            }
            if (x >= mX[n - 1]) {
                return mY[n - 1];
            }
            
            int i = 0;
            while (x >= mX[i + 1]) {
                i += 1;
                if (x == mX[i]) {
                    return mY[i];
                }
            }
            return mY[i] + mM[i] * (x - mX[i]);
        }
      
        public String toString() {
            StringBuilder str = new StringBuilder();
            final int n = mX.length;
            str.append("LinearSpline{[");
            for (int i = 0; i < n; i++) {
                if (i != 0) {
                    str.append(", ");
                }
                str.append("(").append(mX[i]);
                str.append(", ").append(mY[i]);
                if (i < n-1) {
                    str.append(": ").append(mM[i]);
                }
                str.append(")");
            }
            str.append("]}");
            return str.toString();
        }
    }
    
    
    
    
     
