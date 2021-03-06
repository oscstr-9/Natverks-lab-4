/**
 Creates a pixel object for easier handling of each pixels information.

 Version 1.0
 Credit to Oscar Östryd
 2020-10-13
 */
public class Pixel {
    /** Pixel settings to be set*/
    int x, y, color;

        public Pixel(int xIn, int yIn, int colorIn){
            x = 4*xIn;
            y = 4*yIn;
            color = colorIn;
        }

    public int getX() {return x;}

    public int getY() {
        return y;
    }

    public int getColor() {return color;}
}
