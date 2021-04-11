import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.util.ArrayList;

/**
 The server class allows for the program to recieve data from the c++ client.
 It then processes the information recieved and puts it into an ArrayList for
 pixels that it then sends to the PixelCanvas for it to draw onto the canvas.

 Version 1.0
 Credit to Oscar Östryd
 2020-10-13
 */
public class Server {
    /** Variables to be set in order to get the right network settings*/
    private DatagramSocket udpSocket;
    private int port;

    /**
     Has the program set a port and datagram socket.
     */
    public Server(int port) throws SocketException, IOException {
        this.port = port;
        this.udpSocket = new DatagramSocket(this.port);
    }

    /**
     Allows the program to listen for incoming packets of data.
     It also allows the program to handle said data by seeing how many pixels are sent
     and adding them to an ArrayList that is then drawn.
     */
    public void listen(Frame window) throws Exception {
        System.out.println("-- Running Server at " + InetAddress.getLocalHost() + "--");
        ArrayList<Pixel> msg = new ArrayList<>();

        while (true) {

            byte[] buf = new byte[10240];
            DatagramPacket packet = new DatagramPacket(buf, buf.length);

            // blocks until a packet is received
            udpSocket.receive(packet);

            System.out.println(packet);

            msg.clear();
            int amountOfPixels = (Byte.toUnsignedInt(packet.getData()[0])+((packet.getData()[1])<<8));
            System.out.print(amountOfPixels);
            for (int i = 2; i < amountOfPixels*3; i+=3){
                msg.add(new Pixel(Byte.toUnsignedInt(packet.getData()[i]),Byte.toUnsignedInt(packet.getData()[i+1]),Byte.toUnsignedInt(packet.getData()[i+2])));
            }

            window.canvas.Draw(msg);
        }
    }

}