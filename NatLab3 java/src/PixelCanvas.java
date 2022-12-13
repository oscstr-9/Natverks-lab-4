import java.io.IOException;
import javax.swing.*;
import java.awt.*;
import java.util.ArrayList;

/**
 <--THIS IS THE GUI-->

 Allows the pixels to have their color and position set on the canvas
 and for the pixels to be drawn.

 Version 1.0
 Credit to Oscar Östryd
 2020-10-13
 */
public class PixelCanvas extends JComponent {
    /** Used to check what color each pixel should be drawn with*/
    int color;
    /** ArrayList of pixels to be drawn*/
    ArrayList<Pixel> pixel;

    /**
     Sets the pixels color and uses that alongside their position to draw out rectangles
     of that color in the correct location.
     */
    @Override
    protected void paintComponent(Graphics g) {
        super.paintComponent(g);
        if(pixel != null) {
            for (int i = 0; i < pixel.size(); i++) {
                color = pixel.get(i).getColor();
                switch (color) {
                    case 0:
                        g.setColor(new Color(200,175,150));
                        break;
                    case 1:
                        g.setColor(Color.red);
                        break;
                    case 2:
                        g.setColor(Color.green);
                        break;
                    case 3:
                        g.setColor(Color.blue);
                        break;
                    case 4:
                        g.setColor(Color.black);
                        break;
                    case 5:
                        g.setColor(Color.YELLOW);
                        break;
                    case 6:
                        g.setColor(new Color(250,0,150));
                        break;
                    case 7:
                        g.setColor(new Color(0,200,255));
                        break;
                    case 8:
                        g.setColor(Color.cyan);
                        break;
                    default:
                        continue;
                }
                g.fillRect(pixel.get(i).getX(), pixel.get(i).getY(), 4, 4);
            }
        }
    }

    /**
     Updates the pixel ArrayList so that the paintComponent method runs
     and updates the pixels color and position on the canvas.
     */
    public void Draw(ArrayList<Pixel> pixelIn){
        pixel = pixelIn;
        repaint();
    }
}