import javax.swing.*;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;

/**
 Creates a frame for the program to display its contents on.

 Version 1.0
 Credit to Oscar Östryd
 2020-10-13
 */

public class Frame {
    /**
     * frame creates a window for all information to be displayed upon.
     */
    JFrame frame;
    /**
     * float variables for screen size.
     */
    int width = 804, height = 804;
    /**
     * creates a canvas for the pixels to be drawn on
     */
    PixelCanvas canvas = new PixelCanvas();

    public Frame(Server server) {
        frame = new JFrame();

        canvas.setBounds(0, 0, width, height);

        frame.setTitle("Game server of doom");
        frame.setSize(width, height);//804 width and 804 height
        frame.add(canvas);
        frame.setLayout(null);//using no layout managers
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setVisible(true);//making the frame visible
        frame.addKeyListener(new KeyListenerFunc(server));
    }

    /**
     * The main method starts all the neccesary methods for the program to function.
     */
    public static void main(String[] args) throws Exception {
        Server server = new Server(54000);
        Frame window = new Frame(server);
        server.listen(window);
    }

    /**
     * Keylistener that looks for inputs to control one of the players
     */
    public class KeyListenerFunc implements KeyListener {
        Server server;
        public KeyListenerFunc(Server server){
            this.server = server;
        }

        /**
         * checks if and what key has been pressed
         * @param e the pressed key
         */
        public void keyPressed(KeyEvent e) {

            int key = e.getKeyCode();

            if (key == KeyEvent.VK_UP) {
                server.sendInputsToClient((byte)0);
                System.out.print("Up");
            }

            if (key == KeyEvent.VK_LEFT) {
                server.sendInputsToClient((byte)1);
                System.out.print("Left");
            }

            if (key == KeyEvent.VK_DOWN) {
                server.sendInputsToClient((byte)2);
                System.out.print("Down");
            }

            if (key == KeyEvent.VK_RIGHT) {
                server.sendInputsToClient((byte)3);
                System.out.print("Right");
            }
        }
        /**
         * checks if and what key has been pressed
         * @param ev the released key
         */
        public void keyReleased(KeyEvent ev) {

        }
        /**
         * checks if and what key has been pressed
         * @param ev the typed key
         */
        public void keyTyped(KeyEvent ev) {

        }
    }
}