import javax.swing.*;

/**
 Creates a frame for the program to display its contents on.

 Version 1.0
 Credit to Oscar Östryd
 2020-10-13
 */
public class Frame{
    /** frame creates a window for all information to be displayed upon.*/
    JFrame frame;
    /** float variables for screen size.*/
    int width = 804, height = 804;
    /** creates a canvas for the pixels to be drawn on*/
    PixelCanvas canvas = new PixelCanvas();

    public Frame() {
        frame = new JFrame();

        canvas.setBounds(0,0,width,height);

        frame.setTitle("Game server of doom");
        frame.setSize(width, height);//804 width and 804 height
        frame.add(canvas);
        frame.setLayout(null);//using no layout managers
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setVisible(true);//making the frame visible

    }
    /**
     The main method starts all the neccesary methods for the program to function.
     */
    public static void main(String[] args) throws Exception {
        Server server = new Server(54000);
        Frame window = new Frame();
        server.listen(window);
    }
}