package com.test.a4over6_vpn;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.widget.Button;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.VpnService;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.content.Context;
import android.widget.TextView;
import android.widget.Toast;
import android.util.Log;

import java.io.BufferedOutputStream;
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.Inet6Address;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method


        //TODO 检查网络状态，获取IPV6地址
        checkNetStatus();
        //TODO 开启后台线程，调用startBackground()
        final Button connectButton = (Button)findViewById(R.id.ConnectButton);
        connectButton.setOnClickListener(new View.OnClickListener(){
            public void onClick(View view)
            {
                Toast.makeText(MainActivity.this,"kai qi vpn",Toast.LENGTH_LONG).show();
                Runnable background = new Runnable(){
                    public void run()
                    {
                        Log.d("background","start background thread");
                        String temp = startBackground();
                        Log.d("background","return" + temp);

                    }
                };
                Thread backgroundthr = new Thread(background);
                backgroundthr.start();



                //---------------------------------------------------------------------------------------------------------------------------------------------
                Runnable readIpPipe = new Runnable() {
                    @Override
                    public void run() {
                        String extDir = getApplicationInfo().dataDir;
                        Log.d("ykd",extDir.toString());
                        File ipTunnel = new File(extDir,"ip_pipe");
                        if(ipTunnel.exists())
                        {
                            if(ipTunnel.isDirectory())
                            {
                                ipTunnel.delete();
                                try {
                                    ipTunnel.createNewFile();
                                }catch (IOException e){
                                    Log.d("wjf","IOEexception");
                                }

                            }
                        }
                        else
                        {
                            try {
                                ipTunnel.createNewFile();
                            }catch (IOException e){
                                Log.d("wjf","IOEexception");
                            }
                        }
                        try {
                            byte[] buffer = new byte[1024];
                /*String ostr = new String("hahaha");
                buffer = ostr.getBytes();
                FileOutputStream fileOutputStream = new FileOutputStream(ipTunnel);
                BufferedOutputStream out = new BufferedOutputStream(fileOutputStream);
                try{
                    out.write(buffer);
                }catch (IOException e){
                    e.printStackTrace();
                }*/

                            FileInputStream fileInputStream = new FileInputStream(ipTunnel);
                            BufferedInputStream in = new BufferedInputStream(fileInputStream);
                            Log.d("wjf", "Buffered input stream opened");
                            int len = 0;
                            try{
                                while(len <= 0)
                                {
                                    len = in.read(buffer);
                                    if(len > 0)
                                    {
                                        String ret = new String(buffer);
                                        ret = ret.substring(0, len);
                                        Log.d("wjf","ipv4 address and dns: " + ret);

                                    }
                                }
                                in.close();

                            }catch (IOException e){
                                e.printStackTrace();
                            }


                        } catch (FileNotFoundException e) {
                            e.printStackTrace();
                        }
                    }
                };
                Thread readIpPipeth = new Thread(readIpPipe);
                readIpPipeth.start();
            }
        });
        //TODO 开启前台计时器刷新界面

        //创建IP信息管道




        //TODO 创建流量信息管道 info_pipe ，开启VPN服务，将安卓虚接口写入IP信息管道
//        out.write(arr, 0, arr.length)
//        out.flush();
//        out.close();
    }

    private boolean isNetConnected() {
        ConnectivityManager connectivityManager =
                (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        if (connectivityManager != null) {
            NetworkInfo networkInfo = connectivityManager.getActiveNetworkInfo();
            if (networkInfo != null) {
                if (networkInfo.isConnected()) {
                    return true;
                }
            }
        }
        return false;
    }

    // check if wifi connected
    private boolean isWifiConnected() {
        ConnectivityManager connectivityManager = (ConnectivityManager)
                getSystemService(Context.CONNECTIVITY_SERVICE);

        if (connectivityManager != null) {
            NetworkInfo networkInfo = connectivityManager.getActiveNetworkInfo();
            if (networkInfo != null && networkInfo.getType() == ConnectivityManager.TYPE_WIFI) {
                return true;
            }
        }
        return false;
    }

    private void checkNetStatus() {
        TextView statusTextView = (TextView) findViewById(R.id.NetworkStatusTextView);
        if (!isNetConnected()) {
            Toast.makeText(MainActivity.this, "You are not connected to network", Toast.LENGTH_SHORT).show();
            statusTextView.setText("You are not connected to network" + "\n");
            return;
        }
        /*if (!isWifiConnected()) {
            Toast.makeText(MainActivity.this, "You are not connected by WiFi", Toast.LENGTH_SHORT).show();
            statusTextView.setText("You are not connected by WiFi\n");
            return;
        }*/
        statusTextView.setText("You are connected by WiFi\n");

        String macAddr = getMacAddress();
        TextView macTextView = (TextView) findViewById(R.id.MacTextView);
        macTextView.setText(macAddr);
        String ipv6Addr = getIPv6Address();
        TextView textView = (TextView)findViewById(R.id.IPv6AddressTextView);
        textView.setText(ipv6Addr);
        Log.d("wjf", ipv6Addr);
    }

    private String getMacAddress() {
        WifiManager wifiManager = (WifiManager) getApplicationContext().getSystemService(WIFI_SERVICE);
        WifiInfo wifiInfo = wifiManager.getConnectionInfo();
        return wifiInfo.getMacAddress();

    }

    private String getIPv6Address()
    {
        try{
            final Enumeration<NetworkInterface> e = NetworkInterface.getNetworkInterfaces();
            while (e.hasMoreElements())
            {
                final NetworkInterface networkInterface = e.nextElement();
                for(Enumeration<InetAddress>enumAddress = networkInterface.getInetAddresses();enumAddress.hasMoreElements();)
                {
                    InetAddress inetAddress = enumAddress.nextElement();
                    //TODO 检查原来的函数逻辑
//                    if(inetAddress instanceof Inet6Address
//                            && inetAddress.isSiteLocalAddress()
//                            && !inetAddress.isLoopbackAddress()
//                            && !isReservedAddr(inetAddress))
//                    {
//                        String ipAddr = inetAddress.getHostAddress();
//                        String ipAddripAddr = null;
//                        int index = ipAddr.indexOf('%');
//                        if (index > 0) {
//                            ipAddripAddr = ipAddr.substring(0, index);
//                        }
//                        return ipAddr;
//                    }
                    if (!inetAddress.isLoopbackAddress() && !inetAddress.isLinkLocalAddress()) {
                        return inetAddress.getHostAddress();
                    }
                }
            }
        }catch (SocketException e)
        {
            Log.wtf("WIFI_IP", "Unable to NetworkInterface.getNetworkInterfaces()");
        }
        return "";
    }
    private static boolean isReservedAddr(InetAddress inetAddr) {
        if (inetAddr.isAnyLocalAddress() || inetAddr.isLinkLocalAddress()
                || inetAddr.isLoopbackAddress()) {
            return true;
        }

        return false;
    }



    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native String startBackground();
    public native String test();
}
