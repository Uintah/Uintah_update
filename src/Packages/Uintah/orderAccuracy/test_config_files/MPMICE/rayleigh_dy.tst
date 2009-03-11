<start>
<upsFile>rayleigh.ups</upsFile>
<Study>Res.Study</Study>
<gnuplotFile>plotScript.gp</gnuplotFile>

<AllTests>
</AllTests>
<Test>
    <Title>25</Title>
    <sus_cmd>sus </sus_cmd>
    <Study>Res.Study</Study>
    <compare_cmd>compare_Rayleigh.m -pDir 1 -mat 1 -plot false</compare_cmd>
    <x>25</x>
    <replace_lines>
      <resolution>   [10,25,1]          </resolution>
    </replace_lines>
</Test>
<Test>
    <Title>50</Title>
    <sus_cmd>sus </sus_cmd>
    <Study>Res.Study</Study>
    <compare_cmd>compare_Rayleigh.m -pDir 1 -mat 1 -plot false</compare_cmd>
    <x>50</x>
    <replace_lines>
      <resolution>   [10,50,1]          </resolution>
    </replace_lines>
</Test>
<Test>
    <Title>100</Title>
    <sus_cmd>sus </sus_cmd>
    <Study>Res.Study</Study>
    <compare_cmd>compare_Rayleigh.m -pDir 1 -mat 1 -plot false</compare_cmd>
    <x>100</x>
    <replace_lines>
      <resolution>   [10,100,1]          </resolution>
    </replace_lines>
</Test>

<Test>
    <Title>200</Title>
    <sus_cmd>sus </sus_cmd>
    <Study>Res.Study</Study>
    <compare_cmd>compare_Rayleigh.m -pDir 1 -mat 1 -plot false</compare_cmd>
    <x>200</x>
    <replace_lines>
      <resolution>   [10,200,1]          </resolution>
    </replace_lines>
</Test>

<Test>
    <Title>400</Title>
    <sus_cmd>sus </sus_cmd>
    <Study>Res.Study</Study>
    <compare_cmd>compare_Rayleigh.m -pDir 1 -mat 1 -plot false</compare_cmd>
    <x>400</x>
    <replace_lines>
      <resolution>   [10,400,1]          </resolution>
    </replace_lines>
</Test>
<Test>
    <Title>800</Title>
    <sus_cmd>sus </sus_cmd>
    <Study>Res.Study</Study>
    <compare_cmd>compare_Rayleigh.m -pDir 1 -mat 1 -plot false</compare_cmd>
    <x>800</x>
    <replace_lines>
      <resolution>   [10,800,1]          </resolution>
    </replace_lines>
</Test>
</start>
