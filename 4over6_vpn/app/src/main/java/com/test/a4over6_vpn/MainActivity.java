package com.test.a4over6_vpn;

import android.content.Intent;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
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
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

import lecho.lib.hellocharts.model.Axis;
import lecho.lib.hellocharts.model.AxisValue;
import lecho.lib.hellocharts.model.Column;
import lecho.lib.hellocharts.model.ColumnChartData;
import lecho.lib.hellocharts.model.Line;
import lecho.lib.hellocharts.model.LineChartData;
import lecho.lib.hellocharts.model.PointValue;
import lecho.lib.hellocharts.model.SubcolumnValue;
import lecho.lib.hellocharts.model.ValueShape;
import lecho.lib.hellocharts.util.ChartUtils;
import lecho.lib.hellocharts.view.ColumnChartView;
import lecho.lib.hellocharts.view.LineChartView;

public class MainActivity extends AppCompatActivity {
    private static String[] info;
    private static String ipv4Addr;
    private static String router;
    private static String dns1;
    private static String dns2;
    private static String dns3;
    private static String sockfd;
    //TODO /* 界面修改，
      /* 上传、下载速度
             * 上传总流量和包数
             * 下载总流量和包数
             */
    private static String uploadSpeed;
    private static String downloadSpeed;
    private static String uploadTotalPkg;
    private static String downloadTotalPkg;
    private static String uploadTotalLength;
    private static String downloadTotalLength;
    private static final  int INFOLENGTH = 6;
    private static int infoKey = 0;
    private static float uploadSpeedInfo[];
    private static float downloadSpeedInfo[];
    private static float uploadTotalPkgInfo[];
    private static float downloadTotalPkgInfo[];
    private static float uploadTotalLengthInfo[];
    private static float downloadTotalLengthInfo[];
    private static String unit1;
    private static String unit2;


    private boolean IPflag =  true;

    private static final int MSG_UPDATEUI = 0;
    private static final int MSG_CONNECT  = 1;
    private static final int MSG_STATUS   = 2;
    private static final int MSG_DISCONNECT   = 3;
    private static boolean ifConnect;

    private ColumnChartView chart;

    public final static String[] infoLabels = new String[] { "uploadSpeed", "downloadSpeed",
            "uploadTotalLength", "uploadTotalPkg", "downloadTotalLength", "downloadTotalPkg" };

    File infoTunnel;
    FileInputStream infoInputStream;
    BufferedInputStream infoBufferStream;
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }
    private Handler mHandler = new Handler() {
        public void handleMessage (Message msg) { // in main(UI) thread
            TextView editText = (TextView) findViewById(R.id.editText);
//            TextView uploadspeed = (TextView)findViewById(R.id.UploadSpeed);
//            TextView downloadspeed = (TextView)findViewById(R.id.DownloadSpeed);
            TextView uploadLength = (TextView)findViewById(R.id.UploadTotalLength);
            TextView downloadLength = (TextView)findViewById(R.id.DownloadTotalLength);
//            TextView uploadPkg = (TextView)findViewById(R.id.UploadPkg);
//            TextView downloadPkg = (TextView)findViewById(R.id.DownloadPkg);
            switch (msg.what) {
                case MSG_UPDATEUI:
                    Log.d("wjf", "MSG_UPDATEUI");
                    //Done 根据管道数据更新上传速度等信息
                    String update = (String)msg.obj;
                    Log.d("wjf", update);
                    info = update.split(" ");
                    Log.d("wjf","wjf");
                    if (info.length == 8){
                        Log.d("wjf","split 8");

                        uploadSpeed = info[0];
                        float tempfloat[] = new float[6];

                        for(int i=0;i<INFOLENGTH-1;i++)
                        {
                           uploadSpeedInfo[INFOLENGTH-1-i] = uploadSpeedInfo[INFOLENGTH-1-i-1];
                            Log.d("wjf","up"+uploadSpeedInfo[i]);
                        }
                        uploadSpeedInfo[0] = Float.valueOf(info[0]);
                        downloadSpeed = info[1];
                        for(int i=0;i<INFOLENGTH-1;i++)
                        {
                            downloadSpeedInfo[INFOLENGTH-1-i] = downloadSpeedInfo[INFOLENGTH-1-i-1];
                        }
                        downloadSpeedInfo[0] = Float.valueOf(info[1]);
                        uploadTotalLength = info[2];

                        uploadTotalPkg = info[3];
                        for(int i=0;i<INFOLENGTH-1;i++)
                        {
                            uploadTotalPkgInfo[INFOLENGTH-1-i] = uploadTotalPkgInfo[INFOLENGTH-1-i-1];
                        }
                        uploadTotalPkgInfo[0] = Float.valueOf(info[3]);
                        downloadTotalLength = info[4];

                        downloadTotalPkg = info[5];
                        for(int i=0;i<INFOLENGTH-1;i++)
                        {
                            downloadTotalPkgInfo[INFOLENGTH-1-i] = downloadTotalPkgInfo[INFOLENGTH-1-i-1];
                        }
                        downloadTotalPkgInfo[0] = Float.valueOf(info[5]);
                        unit1 = info[6];
                        unit2 = info[7];

                    }


                    //第一条折线+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                    drawLine(uploadSpeedInfo,(LineChartView)findViewById(R.id.uploadSpeedchart),"uploadSpeed");
                    drawLine(downloadSpeedInfo,(LineChartView)findViewById(R.id.downloadspeedchart),"downloadSpeed");
//                    drawLine(uploadTotalLengthInfo,(LineChartView)findViewById(R.id.uploadTotalLengthchart),"uploadTotalLength");
                    drawLine(uploadTotalPkgInfo,(LineChartView)findViewById(R.id.uploadTotalPkgchart),"uploadTotalPkg");
//                    drawLine(downloadTotalLengthInfo,(LineChartView)findViewById(R.id.downloadTotalLengthchart),"downloadTotalLength");
                    drawLine(downloadTotalPkgInfo,(LineChartView)findViewById(R.id.downloadTotalPkgchart),"downloadTotalPkg");

                    //第一条折线+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

                    //TODO 根据获得的上传、下载速度信息更新UI 更好看的界面  https://github.com/lecho/hellocharts-android

                    uploadLength.setText(uploadTotalLength+unit1+"B");
                    downloadLength.setText(downloadTotalLength+unit2+"B");
                    Log.d("wjf","column over");
                    break;
                case MSG_STATUS:
                    String status = (String)msg.obj;
                    Log.d("wjf", status);
                    editText.setText(status + "\n");
                    break;
                case MSG_CONNECT:
                    editText.setText("");
                    String temp = (String)msg.obj;
                    Log.d("wjf", "MSG_CONNECT " + temp);
                    info = temp.split(" ");
                    Log.d("wjf", "info.split length " + info.length);
                    if (info.length == 6) {
                        ipv4Addr = info[0];
                        Log.d("wjf", "ipv4Addr.split length " + ipv4Addr.split("\\.").length);
                        if (ipv4Addr.split("\\.").length == 4) {
                            router   = info[1];
                            dns1     = info[2];
                            dns2     = info[3];
                            dns3     = info[4];
                            sockfd   = info[5];
                            editText.append(ipv4Addr + "\n");
                            editText.append(router + "\n");
                            editText.append(dns1 + "\n");
                            editText.append(dns2 + "\n");
                            editText.append(dns3 + "\n");
                            editText.append(info[5] + "\n");
                            Log.d("wjf", "Will start VPN now");
                            startVPNService();
                        } else {
                            Log.d("wjf", temp);
                        }
                    } else {
                        Log.e("wjf", "Wrong info from server.");
                    }
                    break;
                case MSG_DISCONNECT:
                    editText.setText((String)msg.obj);
                    break;
                default:
                    break;
            }
        }
    };
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
       // chart = (ColumnChartView)findViewById( R.id.chart);
        uploadSpeedInfo = new float[INFOLENGTH];
        downloadSpeedInfo = new float[INFOLENGTH];
        uploadTotalPkgInfo = new float[INFOLENGTH];
        downloadTotalPkgInfo = new float[INFOLENGTH];
        uploadTotalLengthInfo = new float[INFOLENGTH];
        downloadTotalLengthInfo = new float[INFOLENGTH];
        // Example of a call to a native method
        //Done 检查网络状态，获取IPV6地址
        checkNetStatus();
        //TODO 开启后台线程，调用startBackground()
        final Button connectButton = (Button)findViewById(R.id.ConnectButton);
        connectButton.setOnClickListener(new View.OnClickListener(){
            public void onClick(View view)
            {
                Log.d("wjf","kaiqi back");
                Toast.makeText(MainActivity.this,"kai qi background",Toast.LENGTH_LONG).show();

                Runnable background = new Runnable(){
                    public void run()
                    {
                        Log.d("background","start background thread");
                        String temp = startBackground();
                        Log.d("background","return" + temp);
                        Message message = new Message();
                        message.what = MSG_STATUS;
                        message.obj = temp;
                        mHandler.sendMessage(message);
                    }
                };
                Thread backgroundthr = new Thread(background);
               backgroundthr.start();
                //---------------------------------------------------------------------------------------------------------------------------------------------
                Runnable readIpPipe = new Runnable() {
                    @Override
                    public void run() {
                        File ipTunnel = createIpPipe();
                        createInfoPipe();
                        try {
                            byte[] buffer = new byte[1024];
                            FileInputStream fileInputStream = new FileInputStream(ipTunnel);
                            BufferedInputStream in = new BufferedInputStream(fileInputStream);
                            Log.d("wjf", "Buffered input stream opened");
                            int len = 0;
                            try{
                                Log.d("wjf","before read ip tunnel");
                                while(!IsIPTunnelChanged()) {
                                    try {
                                        Thread.sleep(1000);
                                    } catch (InterruptedException e) {
                                        e.printStackTrace();
                                    }
                                }
                                while(IPflag){
                                    len = in.read(buffer);
                                    Log.d("wjf","read ip tunnel"+"wjf"+len);
                                    if(len > 0)
                                    {
                                        Message message = new Message();
                                        String ret = new String(buffer);
                                        ret = ret.substring(0, len);
                                        info = ret.split(" ");
                                        message.obj = ret;
                                        //TODO 通过handler更改UI
                                        if (info.length == 6) {
                                            IPflag = false;
                                            message.what = MSG_CONNECT;
                                        } else {
                                            message.what = MSG_STATUS;
//                                            Log.e("wjf", "Wrong "+ret);
                                        }
                                        mHandler.sendMessage(message);
                                    }
                                }
                                in.close();
                                ifConnect = true;
                                while(ifConnect)
                                {
                                    try {
                                        Thread.sleep(1000);
                                    } catch (InterruptedException e) {
                                        e.printStackTrace();
                                    }
                                    readInfoPipe();
                                }

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

        //TODO 创建流量信息管道 info_pipe ，开启VPN服务，将安卓虚接口写入IP信息管道
//        out.write(arr, 0, arr.length)
//        out.flush();
//        out.close();
        final Button disConnectButton = (Button)findViewById(R.id.DisConnectButton);
        disConnectButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                IPflag = true;
                Message message = new Message();
                message.what = MSG_DISCONNECT;
                message.obj = "disconnect";
                mHandler.sendMessage(message);
                ifConnect = false;
                disConnect();
            }
        });
    }
    private void drawLine(float infoArray[],LineChartView chartview,String title)
    {
        List<Line> lines = new ArrayList<Line>();
        for (int i = 0; i < 1; ++i) {

            List<PointValue> values = new ArrayList<PointValue>();
            for (int j = 0; j < INFOLENGTH; ++j) {
                values.add(new PointValue(j,infoArray[j]));
            }

            Line line = new Line(values);
            line.setColor(ChartUtils.COLORS[i]);
            line.setShape(ValueShape.CIRCLE);
            line.setCubic(false);
            line.setFilled(false);
            line.setHasLabels(false);
            line.setHasLabelsOnlyForSelected(false);
            line.setHasLines(true);
            line.setHasPoints(true);
            if (true) {
                line.setPointColor(ChartUtils.COLORS[(i + 1)
                        % ChartUtils.COLORS.length]);
            }
            lines.add(line);
        }


        LineChartData data = new LineChartData(lines);
        if (true) {
            Axis axisX = new Axis();
            Axis axisY = new Axis().setHasLines(true);

            if (true) {
                axisX.setName("time X");
                axisY.setName(title+" Y");
            }
            data.setAxisXBottom(axisX);
            data.setAxisYLeft(axisY);
        } else {
            data.setAxisXBottom(null);
            data.setAxisYLeft(null);
        }
        LineChartView uploadSpeedChart = chartview;
        data.setBaseValue(Float.NEGATIVE_INFINITY);
        uploadSpeedChart.setLineChartData(data);
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
    private boolean startVPNService(){
        Log.d("wjf","start");
        Intent intent = VpnService.prepare(this);
        if(intent != null)
        {
            startActivityForResult(intent , 0);
        }
        else
        {
            Log.d("wjf","no remind");
            onActivityResult(0,RESULT_OK,null);
        }

        return true;
    }
    protected void onActivityResult(int request,int result,Intent data)
    {
        if(result == RESULT_OK)
        {
            Log.d("wjf","result_ok");
            Intent intent = new Intent(MainActivity.this,MyVPNService.class);
            intent.putExtra("ipv4Addr", ipv4Addr);
            intent.putExtra("router", router);
            intent.putExtra("dns1", dns1);
            intent.putExtra("dns2", dns2);
            intent.putExtra("dns3", dns3);
            intent.putExtra("sockfd", sockfd);
            Log.d("wjf","startService()");
            startService(intent);
        }
    }

    private File createIpPipe(){
        String extDir = getApplicationInfo().dataDir;
        Log.d("wjf",extDir.toString());
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
        return ipTunnel;
    }

    private void createInfoPipe(){
        String extDir = getApplicationInfo().dataDir;
        Log.d("wjf",extDir.toString());
        infoTunnel = new File(extDir,"info_pipe");
        if(infoTunnel.exists())
        {
            if(infoTunnel.isDirectory())
            {
                infoTunnel.delete();
                try {
                    infoTunnel.createNewFile();
                }catch (IOException e){
                    Log.d("wjf","IOEexception");
                }
            }
        }
        else
        {
            try {
                infoTunnel.createNewFile();
            }catch (IOException e){
                Log.d("wjf","IOEexception");
            }
        }

//        try{
//            infoInputStream = new FileInputStream(infoTunnel);
//            infoBufferStream = new BufferedInputStream(infoInputStream);
//        }catch (FileNotFoundException e){
//            e.printStackTrace();
//        }
    }

    private String readInfoPipe(){
        byte[] buffer = new byte[1024];
        int len ;
        try {
            infoInputStream = new FileInputStream(infoTunnel);
            infoBufferStream = new BufferedInputStream(infoInputStream);
            len = infoBufferStream.read(buffer);
            if (len > 0) {
                Message message = new Message();
                String ret = new String(buffer);
                ret = ret.substring(0, len);
                info = ret.split(" ");
                message.obj = ret;
                if (info.length == 8) {
                    message.what = MSG_UPDATEUI;
                } else {
                    message.what = MSG_STATUS;
                }
                mHandler.sendMessage(message);
            }
            infoInputStream.close();
            infoBufferStream.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return "";
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String startBackground();
    public native String test();
    public native boolean IsIPTunnelChanged();
    public native void disConnect();
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                Runnable charrun = new Runnable() {
//                    @Override
//                    public void run() {
//
//                        int t = 1;
//                        while(t>0)
//                        {
//                            Message message = new Message();
//                            message.what = MSG_UPDATEUI;
//                            try{
//                                Thread.sleep(1000);
//                            } catch (InterruptedException e) {
//                                e.printStackTrace();
//                            }
//
//                            String temp = new String(""+t+" "+t+" "+t+" "+t+" "+t+" "+t);
//                            Log.d("wjf","temp"+temp);
//                            message.obj = temp;
//
//                            mHandler.sendMessage(message);
//                            t++;
//                        }
//
//
//                    }
//                };


//                    Thread charthr = new Thread(charrun);
//                    charthr.start();




//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//柱状图++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    int numSubcolumns = 1;
//                    int numColumns = 6;
//                    List<Column> columns = new ArrayList<Column>();
//                    List<SubcolumnValue> values;
//                    List<AxisValue> axisValues = new ArrayList<AxisValue>();
//                    for (int i = 0; i < numColumns; ++i) {
//
//                        values = new ArrayList<SubcolumnValue>();
//                        for (int j = 0; j < numSubcolumns; ++j) {
//                            values.add(new SubcolumnValue(
//                                    Float.valueOf(info[i]),ChartUtils
//                                    .pickColor()));
//                        }
//
//                        Column column = new Column(values);
//                        column.setHasLabels(true);
//
//                        column.setHasLabelsOnlyForSelected(true);
//                        columns.add(column);
//                        axisValues.add(new AxisValue(i).setLabel(infoLabels[i]));
//                    }
//
//                    data = new ColumnChartData(columns);
//                    Axis axisX = new Axis(axisValues);
//                    Axis axisY = new Axis().setHasLines(true);
//                    if (true) {
//                        axisX.setName("Axis Xwjf");
//                        axisY.setName("Axis Ywjf");
//                    }
//                    data.setAxisXBottom(axisX);
//                    data.setAxisYLeft(axisY);
//
//                    chart.setColumnChartData(data);

//柱状图+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++